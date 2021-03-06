using namespace std;

#include <iostream>
#include <vector>
#include <algorithm>				// for binary_search(), lower_bound(), inplace_merge()
#include <string>
#include <fstream>
#include <ctime>
#include <cmath>				// for (TODO: get list of things I need cmath for)
#include <cctype>				// for size_t
#include <errno.h>

#include <CmdLineUtly.h>			// for ProcessFlatCommandLine()
#include <StrUtly.h>				// for TakeDelimitedList(), ToUpper(), StrToDouble(), Size_tToStr()
#include <Utils/CAFE_SQLUtly.h>			// for InsertEvent(), SaveLonLatAnom(), EstablishConnection()
#include <Config/Configuration.h>

#include <Utils/CAFE_CmdLine.h>			// for generic CAFE command line option handling...
#include <Utils/LonLatAnom.h>			// for LonLatAnom structure
#include <Utils/PeakValleyFile.h>
#include <Config/CAFEState.h>

#include <TimeUtly.h>				// for RoundHour() and others
#include <FormatUtly.h>				// for Bold(), Underline()

#include <mysql++/mysql++.h>



bool ProcessAndPrint(const string &ProgOutputName, const set<string> &AllCAFEVarLabels, const time_t &EventDateTime,
		     const double &EventLon, const double &EventLat, const string &EventType,
		     mysqlpp::Query &TheQuery, const CAFEState &currState)
// This funcrion will save the data to the mysql database.
{
	PeakValleyFile extremumStream(ProgOutputName.c_str(), ios::in);

	if (!extremumStream.is_open())
	{
		cerr << "ERROR: Could not open the program's output file: " << ProgOutputName << "  for reading.\n";
		return(false);
	}

	const vector<string> extremumNames = currState.Extremum_Names();

	vector<LonLatAnom> extremumInfo = extremumStream.RetrieveExtrema(AllCAFEVarLabels.size(), extremumNames.size());

	extremumStream.close();

	if (extremumInfo.empty())
	{
		cerr << "ERROR: Problem with reading the peak valley file: " << ProgOutputName << endl;
		return(false);
	}


	// TODO: Refactor this into the TrainingInterface class, somehow...
	vector<string> ColumnNames(AllCAFEVarLabels.size() * extremumNames.size());

	size_t PosOffset = 0;
	for (set<string>::const_iterator CAFELabel = AllCAFEVarLabels.begin();
	     CAFELabel != AllCAFEVarLabels.end();
	     CAFELabel++)
	{
		// PosOffset will be incremented by this for-loop
		for (size_t ExtremumIndex = 0; ExtremumIndex < extremumNames.size(); ExtremumIndex++, PosOffset++)
		{
			ColumnNames[PosOffset] = *CAFELabel + '_' + extremumNames[ExtremumIndex];
		}
	}

	if (!UpdateTable(extremumInfo, ColumnNames, EventDateTime, TheQuery, EventType))
        {
	        cerr << "ERROR: Could not add event information into database for Event Type: " << EventType << endl;
                return(false);
        }

	return(true);
}



//------------------------------------------------------------------------------------------------------------------------------
// These two functions, MakeCmdLineArgs() and RunPeakValExtractor() will need to be modified for 
// whatever particular extractor program that you have.
string MakeCmdLineArgs(CAFEState &currInfo)
{
	const string DataVarName = currInfo.DataVar_Name();

        string LevelArgs = "";
	size_t LevelWrdCnt = 0;
        for (currInfo.EventLevels_Begin(); currInfo.EventLevels_HasNext(); currInfo.EventLevels_Next())
        {
		const vector<string> theLevWrds = TakeDelimitedList(currInfo.DataLevel_Name(), ',');
		LevelWrdCnt += theLevWrds.size();
                LevelArgs += "'" + GiveDelimitedList(theLevWrds, "' '") + "' ";
        }

	return("'" + DataVarName + "' " + Size_tToStr(LevelWrdCnt) + ' ' + LevelArgs);
}

bool RunPeakValExtractor(const string &ProgName, const string &CmdLineArg, const size_t VarCount, const CAFEDomain &TheDomain,
			 const string &DateTimeStr, const string &ProgOutputName)
// Could an injection attack happen here?  It would be pretty difficult, but possibly feasible.
{
	const vector<float> TheLons = TheDomain.GiveLons();
	const vector<float> TheLats = TheDomain.GiveLats();

	const string SysCommand = ProgName + ' ' + DateTimeStr + ' ' + Size_tToStr(VarCount) + ' '
				  + FloatToStr(TheLats[0]) + ' ' + FloatToStr(TheLons[0]) + ' '
				  + FloatToStr(TheLats[1]) + ' ' + FloatToStr(TheLons[1]) + ' '
				  + CmdLineArg + " >& " + ProgOutputName;

//	cerr << SysCommand << endl;
	return(system(SysCommand.c_str()) == 0);
}
//-----------------------------------------------------------------------------------------------------------------------------


void PrintSyntax(const CmdOptions &CAFEOptions)
{
	cout << "\nAnomalyCollection " << Bold("[--offset=") << Underline("Hour_Offset") << Bold("]")
	     << " " << Bold("[--round=") << Underline("Rounding_Value") << Bold("]") << endl;

	CAFEOptions.PrintSyntax(17, 63);

	cout << "                  " << Bold("[--help] [--syntax] ") << Underline("Input_Date_File") << endl;
	cout << endl;
}

void PrintHelp(const CmdOptions &CAFEOptions)
{
	PrintSyntax(CAFEOptions);

	cout << "Collects the peaks and the valleys and stores them away..." << endl;

	cout << endl << Bold("DESCRIPTION") << endl;

	CAFEOptions.PrintDescription(63);

	cout << endl;
}

int main(int argc, char *argv[])
{
	// Working towards eliminating the need for this statement.
	setenv("TZ", "UTC UTC", 1);		// set's the computer's locale to UTC, so all conversions are done there.

	vector <string> CommandArgs = ProcessFlatCommandLine(argc, argv);
	CmdOptions CAFEOptions;

	if (CAFEOptions.ParseCommandArgs(CommandArgs) != 0)
	{
		cerr << "ERROR: Invalid syntax..." << endl;
		PrintSyntax(CAFEOptions);
		return(8);
	}

	string InputFile = "";
	struct TimeIncrement_t TimeOffset;		// by default, it is zero offset.
	int RoundingHourValue = 6;

	for (size_t ArgIndex = 0; ArgIndex < CommandArgs.size(); ArgIndex++)
	{
		if (CommandArgs[ArgIndex].find("--offset=") == 0)
		{
			string TempHold = CommandArgs[ArgIndex].substr(9);

			errno = 0;
			TimeOffset.HourInterval = strtol(TempHold.c_str(), NULL, 10);

			if (errno != 0)
			{
				perror("Syntax Error! (--offset=)");
				return(8);
			}
		}
		else if (CommandArgs[ArgIndex].find("--round=") == 0)
		{
			string TempHold = CommandArgs[ArgIndex].substr(8);
			
			errno = 0;
			RoundingHourValue = strtol(TempHold.c_str(), NULL, 10);

			if (errno != 0)
			{
				perror("Syntax Error! (--round=)");
				return(8);
			}

			if (RoundingHourValue <= 0)
			{
				cerr << "Syntax Error! (--round=): Rounding value cannot be zero or negative.\n";
				return(8);
			}
		}
		else if (CommandArgs[ArgIndex] == "--help")
		{
			PrintHelp(CAFEOptions);
			return(2);
		}
		else if (CommandArgs[ArgIndex] == "--syntax")
		{
			PrintSyntax(CAFEOptions);
			return(2);
		}
		else
		{
			InputFile = CommandArgs[ArgIndex];
		}
	}

	if (InputFile.empty())
	{
		cerr << "\n\tImproper syntax:  must have the filename of the event list file\n\n";
		PrintSyntax(CAFEOptions);
		return(8);
	}

	Configuration ConfigInfo( CAFEOptions.CAFEPath + '/' + CAFEOptions.ConfigFilename );

	if (!ConfigInfo.ValidConfig())
	{
		cerr << "ERROR: Invalid configuration file: " << CAFEOptions.CAFEPath << '/' << CAFEOptions.ConfigFilename << endl;
		return(1);
	}

	if (!CAFEOptions.MergeWithConfiguration(ConfigInfo))
        {
                cerr << "ERROR: Conflicts in the command line..." << endl;
		PrintSyntax(CAFEOptions);
                return(8);
        }

	if (!ConfigInfo.SetDefaultSource("NARR"))
	{
		cerr << "ERROR: Couldn't set NARR as a default data source\n";
		return(1);
	}

	CAFEState currState( CAFEOptions.ConfigMerge( ConfigInfo.GiveCAFEInfo() ) );

	if (!currState.TimePeriods_JumpTo(TimeOffset.HourInterval))
	{
		cerr << "ERROR: Invalid timeperiod: " << TimeOffset.HourInterval << endl;
		return(8);
	}


	const string PeakValoutputName = currState.GetCAFEPath() + "/scratch/tempdata.txt";
	const string PeakValProgName = currState.GetCAFEPath() + "/bin/PeakValleyPicker";

	vector <double> EventLons;
	vector <double> EventLats;
	vector <time_t> EventDates;
	vector <string> EventTypes;
	
	ifstream EventFile(InputFile.c_str());

	if (!EventFile.is_open())
	{
		cerr << "\n\tProblem opening the event file: " << InputFile << endl;
		return(9);
	}

	string LineRead;

	getline(EventFile, LineRead);

	while (!EventFile.eof())
	{
		vector <string> TempHold = TakeDelimitedList(LineRead, ',');

		if (TempHold.size() == 4)	// Date, type, lat, lon
		{
			if (!ConfigInfo.ValidEventType(TempHold[1]))
        	        {
				// the event type doesn't even exist in the configuration file.
                        	cerr << "ERROR: Invalid event type: " << TempHold[1] << endl;
	                        EventFile.close();
        	                return(9);
                	}
			else if (currState.EventType_Check(TempHold[1]))
			{
				// the event type is an event type indicated by given command-line
				// options or by the defaults (config file) if no options are given
				time_t TheDateTime = GetTimeUTC(TempHold[0]);
				TheDateTime = IncrementDateTime(TheDateTime, TimeOffset);	// by default, no change occurs
				RoundHour(TheDateTime, RoundingHourValue);			// round to the nearest RoundingHourValue hours, starting at 00Z.
				EventDates.push_back(TheDateTime);
				EventTypes.push_back(TempHold[1]);
				EventLats.push_back(StrToDouble(TempHold[2]));
				EventLons.push_back(StrToDouble(TempHold[3]));
			}
		}
		else
		{
			cerr << "ERROR: Invalid line in the event file: " << LineRead << endl
			     << "       Must be a comma delimited list of date, event type, lat, and lon.\n"
			     << "       Skipping...\n";
		}

		
		getline(EventFile, LineRead);
	}

	EventFile.close();

	cout << "Number of Events: " << EventDates.size() << endl;

	if (EventDates.empty())
	{
		cout << "No events to do!" << endl;
		return(0);
	}

	mysqlpp::Connection ServerLink;

        try
        {
		EstablishConnection(ServerLink, currState.GetServerName(), currState.GetLoginUserName(), currState.Untrained_Name());
	}
	catch (const string &ErrStr)
	{
		cerr << "ERROR: Connection failed: " << ErrStr << endl;
		ServerLink.close();
		return(3);
	}
        catch (const exception& Err)
        {
                cerr << "ERROR: Connection failed: " << Err.what() << endl;
		ServerLink.close();
                return(3);
        }
        catch (...)
        {
                cerr << "ERROR: Unknown error..." << endl;
		ServerLink.close();
                return(4);
        }

	try
	{
		mysqlpp::Query TheQuery = ServerLink.query();

		for (size_t DateIndex = 0; DateIndex < EventDates.size(); DateIndex++)
		{
			const string GradDateTimeStr = GiveTimeUTC(EventDates[DateIndex], "%HZ%d%b%Y");
			const string DateTimeStr = GiveTimeUTC(EventDates[DateIndex], "%Y-%m-%d %H:%M:%S");

			cout << "EventType: " << EventTypes[DateIndex] << " for index: " << DateIndex << "  Date: " << DateTimeStr << endl;
			
			InsertEvent(EventDates[DateIndex], EventLons[DateIndex], EventLats[DateIndex], TheQuery, EventTypes[DateIndex]);

			currState.EventTypes_JumpTo(EventTypes[DateIndex]);
			
			set<string> allCAFEVarLabels;
			string CmdLineArg = "";
			
			for (currState.EventVars_Begin(); currState.EventVars_HasNext(); currState.EventVars_Next())
			{
				set<string> eventFields;
				for (currState.EventLevels_Begin(); currState.EventLevels_HasNext(); currState.EventLevels_Next())
				{
					const set<string> fieldNames = currState.EventField_Names();
					eventFields.insert(fieldNames.begin(), fieldNames.end());
				}
				
				allCAFEVarLabels.insert(eventFields.begin(), eventFields.end());

				CmdLineArg += MakeCmdLineArgs(currState) + ' ';
			}// end CAFEVar loop
			
			if (!RunPeakValExtractor(PeakValProgName, CmdLineArg, currState.EventVars_Size(), currState.GetCAFEDomain(),
			                         GradDateTimeStr, PeakValoutputName))
                        {
                                throw("Unable to execute the PeakValley collection. EventIndex: " + Size_tToStr(DateIndex));
                        }

			if (!ProcessAndPrint(PeakValoutputName, allCAFEVarLabels, 
					     EventDates[DateIndex], EventLons[DateIndex], EventLats[DateIndex],
					     EventTypes[DateIndex], TheQuery, currState))
                        {
                                throw("Unable to ProcessAndPrint(). EventIndex: " + Size_tToStr(DateIndex));
                        }
		}// end date index loop



		const set<string> uniqueEventTypes(EventTypes.begin(), EventTypes.end());

		ServerLink.select_db(currState.Trained_Name());
		for (set<string>::const_iterator AnEventType = uniqueEventTypes.begin(); 
		     AnEventType != uniqueEventTypes.end(); 
		     AnEventType++)
		{
			TheQuery << "INSERT IGNORE " << *AnEventType << " (DateInfo, Event_Lon, Event_Lat) "
				 << "SELECT DateInfo, Event_Lon, Event_Lat FROM " << currState.Untrained_Name() << '.' << *AnEventType;

			TheQuery.execute();

			if (!TheQuery.success())
			{
				throw("Unable to insert event information to the clustered database: " + currState.Trained_Name()
				      + " for event type: " + *AnEventType);
			}
		}
	}
	catch (const exception &Err)
	{
		cerr << "ERROR: Exception caught.  Message: " << Err.what() << endl;
		ServerLink.close();
		return(5);
	}
	catch (const string &ErrorStr)
	{
		cerr << "ERROR: " << ErrorStr << endl;
		ServerLink.close();
		return(6);
	}
	catch (...)
	{
		cerr << "ERROR: Unknown exception caught." << endl;
		ServerLink.close();
		return(7);
	}
		
	ServerLink.close();
	return(0);
}

