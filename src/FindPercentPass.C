using namespace std;

#include <iostream>
#include <iomanip>			// for printf()
#include <fstream>
#include <string>
#include <vector>
#include <cmath>			// for sqrt()
#include <cctype>			// for size_t

#include "Config/Configuration.h"
#include "Config/CAFEState.h"

#include "Utils/CAFEUtly.h"			// for LoadEventScores()
#include <StrUtly.h>			// for TakeDelimitedList(), StrToDouble(), StripWhiteSpace()


#include <CmdLineUtly.h>                // for ProcessFlatCommandLine()
#include "Utils/CAFE_CmdLine.h"               // for generic CAFE command line handling...
#include <FormatUtly.h>                 // for making things look nice and pretty

#define _PROG_NAME              ((string) "FindPercentPass")


void PrintSyntax(const CmdOptions &CAFEOptions)
{
	string TheSpacing((_PROG_NAME).size() + 6, ' ');
        cout << "\n\n";
        cout << Bold("NAME") << endl;
        cout << "     " + _PROG_NAME + " - displays statistics for the results of a particular hindcast.\n";
        cout << Bold("SYNOPSIS") << endl;
        cout << "     ";
        cout << Bold(_PROG_NAME) + Bold(" [--help] [--syntax] [--prefix=") + Underline("TypePrefix") + Bold("]") << "\n";
#ifdef _SAMPLINGRUN_
        cout << TheSpacing << Bold("--case=") << Underline("CASE_NUM") << "\n";
#endif
        CAFEOptions.PrintSyntax((_PROG_NAME).size() + 5, 63);
        cout << endl;
}


void PrintHelp(const CmdOptions &CAFEOptions)
{
        PrintSyntax(CAFEOptions);

        cout << Bold("DESCRIPTION") << endl;

        cout << "               This program will analyze the results of the latest hindcast\n";
	
        cout << Bold("OPTIONS") << endl;

        cout << Bold("\t--help --syntax") << endl << endl;

	cout << Bold("\t--prefix") << endl;
	cout << "\t\tan optional prefix specifier to use if the event scores are not from the usual hindcast.\n\n";

#ifdef _SAMPLINGRUN_
        cout << Bold("\t--fold") << endl;
	cout << "\t\tspecify which case is being dealt with so that the correct event scores and thresholds are obtained.\n\n";
#endif


        CAFEOptions.PrintDescription(63);

        cout << "\n\n";
}




vector <size_t> GenerateTruthTable(const vector <double> &EventScore, const double &ThresholdVal)
{
	vector <size_t> TruthTable(2, 0);			// each element counts the number of event scores that fall in to each catagory

	for (vector<double>::const_iterator AScore( EventScore.begin() ); AScore != EventScore.end(); AScore++)
	{
		if (*AScore >= ThresholdVal)
		{
			TruthTable[0]++;		// the event did happen AND the system recognized it		    (good)
		}
		else
		{
			TruthTable[1]++;		// the event did happen AND the system did NOT recognize it         (bad)
		}
	}

	return(TruthTable);

}


int main(int argc, char *argv[])
{
        vector <string> CommandArgs = ProcessFlatCommandLine(argc, argv);
        CmdOptions CAFEOptions;

        if (CAFEOptions.ParseCommandArgs(CommandArgs) != 0)
        {
                cerr << "ERROR: Invalid syntax..." << endl;
                PrintSyntax(CAFEOptions);
                return(8);
        }

#ifdef _SAMPLINGRUN_
        int FoldNumber = -1;
#endif

	string OptionalPrefix = "";

        for (size_t ArgIndex = 0; ArgIndex < CommandArgs.size(); ArgIndex++)
        {
                if (CommandArgs[ArgIndex] == "--help")
                {
                        PrintHelp(CAFEOptions);
                        return(2);
                }
                else if (CommandArgs[ArgIndex] == "--syntax")
                {
                        PrintSyntax(CAFEOptions);
                        return(2);
                }
		else if (CommandArgs[ArgIndex].find("--prefix=") == 0)
		{
			OptionalPrefix = CommandArgs[ArgIndex].substr(9);
		}
#ifdef _SAMPLINGRUN_
                else if (CommandArgs[ArgIndex].find("--fold=") == 0)
                {
                        FoldNumber = StrToInt( CommandArgs[ArgIndex].substr(7) );
                }
#endif
                else
                {
                        cerr << "ERROR: Unknown option: " << CommandArgs[ArgIndex] << endl;
                        PrintSyntax(CAFEOptions);
                        return(8);
                }
        }

#ifdef _SAMPLINGRUN_
	if (FoldNumber <= 0 || FoldNumber > 10)
	{
		cerr << "ERROR: Invalid fold number.  The value was interpreated as: " << FoldNumber << endl;
		cerr << "Note: An invalid conversion from string to int returns a zero." << endl;
		return(8);
	}
#endif

        Configuration ConfigInfo(CAFEOptions.CAFEPath + '/' + CAFEOptions.ConfigFilename);

        if (!ConfigInfo.IsValid())
        {
                cerr << "ERROR: Something wrong with the config file: " << CAFEOptions.CAFEPath << '/' << CAFEOptions.ConfigFilename << endl;
                return(1);
        }

        if (!CAFEOptions.MergeWithConfiguration(ConfigInfo))
        {
                cerr << "ERROR: Conflicts in the command line..." << endl;
                PrintSyntax(CAFEOptions);
                return(8);
        }

	CAFEState currState( CAFEOptions.ConfigMerge( ConfigInfo.GiveCAFEInfo() ) );

	const string BaseDir = currState.GetCAFEPath() + "/AnalysisInfo/";

	try
	{
        	for (currState.TimePeriods_Begin(); currState.TimePeriods_HasNext(); currState.TimePeriods_Next())
        	{
			cout << endl;
			vector <string> TheTableList;
			vector <double> ThresholdVals;

#ifndef _SAMPLINGRUN_
	                const string ThresholdFileName = BaseDir + currState.Trained_Name() + "/ThresholdVals.dat";
#else
        	        const string ThresholdFileName = BaseDir + currState.Trained_Name() + "/ThresholdVals_Fold_" + IntToStr(FoldNumber) + ".dat";
#endif

			if (!LoadThresholdVals(ThresholdFileName, TheTableList, ThresholdVals))
			{
				throw("Could not load the threshold values from file: " + ThresholdFileName);
			}

               	        for (currState.EventTypes_Begin(); currState.EventTypes_HasNext(); currState.EventTypes_Next())
                        {
				if (find(TheTableList.begin(), TheTableList.end(), currState.EventType_Name()) == TheTableList.end())
				{
					cerr << "We do not have the threshold value for this event type: " << currState.EventType_Name() << endl;
					continue;
				}

				const size_t ThreshIndex = find(TheTableList.begin(), TheTableList.end(), currState.EventType_Name()) - TheTableList.begin();

				vector <double> EventScores(0);
				vector <string> EventDates(0);

				string EventScoreFilename = BaseDir + "/CorrelationCalcs/" + currState.Trained_Name() + "/";

				EventScoreFilename += (!OptionalPrefix.empty()) ? (OptionalPrefix + '.') : "";

#ifdef _SAMPLINGRUN_
				EventScoreFilename += currState.EventType_Name() + "_EventScore_Fold_" + IntToStr(FoldNumber) + ".csv";
#else
				EventScoreFilename += currState.EventType_Name() + "_EventScore.csv";
#endif

				if (!LoadEventScores(EventScores, EventDates, EventScoreFilename, currState.EventType_Name()))
				{
					throw("Could not read from the event scores file: " + EventScoreFilename);
				}

				vector <size_t> TruthTable = GenerateTruthTable(EventScores, ThresholdVals[ThreshIndex]);

				printf("Time Period: %-5s  Table: %s\n", currState.TimePeriod_Name().c_str(), 
									 currState.EventType_Name().c_str());
				printf("                   Passed: %%%5.3f    Count: %zu\n", 
				       100.0 * ((double) TruthTable[0]) / ((double) EventScores.size()),
				       TruthTable[0]);
				printf("                   Failed: %%%5.3f    Count: %zu\n",
				       100.0 * ((double) TruthTable[1]) / ((double) EventScores.size()),
				       TruthTable[1]);
				cout << endl;
			}// end Table for-loop
		}// end database for-loop
	}
        catch (const exception &Err)
        {
                cerr << "ERROR: Something went wrong: " << Err.what() << endl;
                return(5);
        }
        catch (const string &ErrStr)
        {
                cerr << "ERROR: " << ErrStr << endl;
                return(6);
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught..." << endl;
                return(7);
        }

	return(0);
}
