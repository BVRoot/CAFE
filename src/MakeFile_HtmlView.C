using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <cctype>			// for size_t
#include <StrUtly.h>			// for RipWhiteSpace(), TakeDelimitedList()

#include "Config/Configuration.h"
#include "Config/CAFEState.h"

#include <mysql++/mysql++.h>
#include "Utils/CAFE_SQLUtly.h"		// for MemberCnt(), EstablishConnection()

#include "Utils/CAFE_CmdLine.h"		// for generic CAFE command line support and control
#include <CmdLineUtly.h>		// for ProcessFlatCommandLine()

void PrintSyntax(const CmdOptions &CAFEOptions)
{
        cout << endl;
        cout << "MakeFile_HtmlView [--help] [--syntax] [--webloc=_WEBLOCATION_]" << endl;

        CAFEOptions.PrintSyntax(17, 63);

        cout << endl;
}

void PrintHelp(const CmdOptions &CAFEOptions)
{
        PrintSyntax(CAFEOptions);

        cout << endl;
        cout << "Makes the html file to view the current CAFE training..." << endl << endl;

        cout << "DESCRIPTION" << endl;
	cout << "--webloc\n"
	     << "     The location for the html files.  Default: ./\n\n";
        CAFEOptions.PrintDescription(63);
        cout << endl;
}

void MakeHeaders(ofstream &HTMLStream, const string &TimePeriod)
{
	HTMLStream << "<html>" << endl;
	HTMLStream << "<body>" << endl;
	HTMLStream << "<p align='center'><b><font style='font-size: 11pt'>" << TimePeriod << "</font></b></p>" << endl;
}

void MakeAlphaPhiPane(ofstream &HTMLStream, const int Height, const int Width, const int RowSpan, const int ColSpan, const string &DefaultSource)
{
	HTMLStream << "   <td width='" << Width << "' height='" << Height << "' rowspan='" << RowSpan << "' colspan='" << ColSpan << "'>\n"
        	   << "      <p align='left'>Alpha: &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Phi:</p>\n"
                   << "      <iframe name='AlphaPhiPane' src='" << DefaultSource << "' height='" << Height/2 << "' width='"
                   << Width << "' marginheight='0' marginwidth='0'>\n"
                   << "      </iframe>\n"
        	   << "      <p align='left'>Max Gamma: &nbsp;&nbsp;&nbsp;&nbsp;Max Chi:</p>\n"
        	   << "   </td>" << endl;
}

void MakeHistoPane(ofstream &HTMLStream, const int Height, const int Width, const int RowSpan, const int ColSpan, const string &DefaultSource)
{
	HTMLStream << "   <td width='" << Width << "' height='" << Height << "' rowspan='" << RowSpan << "' colspan='" << ColSpan << "'>\n"
        	   << "      <iframe name='HistoPane' src='" << DefaultSource << "' height='" << Height << "' width='"
                   << Width << "' marginheight='0' marginwidth='0'>\n"
        	   << "      </iframe>\n"
        	   << "   </td>" << endl;
}

void MakeImagePane(ofstream &HTMLStream, const int Height, const int Width, const int RowSpan, const int ColSpan, const string &DefaultSource)
{
	HTMLStream << "   <td width='" << Width << "' height='" << Height << "' rowspan='" << RowSpan << "' colspan='" << ColSpan << "'>\n"
        	   << "      <iframe name='ImagePane' src='" << DefaultSource << "' height='" << Height << "' width='"
                   << Width << "' marginheight='0' marginwidth='0'>\n"
        	   << "      </iframe>\n"
        	   << "   </td>" << endl;
}

void MakeListPane(ofstream &HTMLStream, const int Height, const int Width, const int RowSpan, const int ColSpan, const string &DefaultSource)
{
	HTMLStream << "   <td width='" << Width << "' height='" << Height << "' rowspan='" << RowSpan << "' colspan='" << ColSpan << "'>\n"
        	   << "      <iframe name='ListPane' src='" << DefaultSource << "' height='" << Height << "' width='"
                   << Width << "' marginheight='0' marginwidth='0'>\n"
                   << "      </iframe>\n"
        	   << "   </td>" << endl;
}

void MakeEventPane(ofstream &HTMLStream, const int Height, const int Width, const int RowSpan, const int ColSpan, const string &DefaultSource)
{
	HTMLStream << "   <td width='" << Width << "' height='" << Height << "' rowspan='" << RowSpan << "' colspan='" << ColSpan << "'>\n"
        	   << "      <iframe name = 'EventPane' src='" << DefaultSource << "' height='" << Height << "' width='"
                   << Width <<"' marginheight='0' marginwidth='0'>\n"
        	   << "      </iframe>\n"
        	   << "   </td>" << endl;
}




void MakeMainFile(const string HTMLName, const set<string> &TimePeriods)
{
	ofstream HTMLStream(HTMLName.c_str());

        if (!HTMLStream.is_open())
        {
	        throw("Could not open this file: " + HTMLName);
        }

	int EventPaneWidth = 200;
	int EventPaneHeight = 738;
	int ListPaneWidth = 290;
	int ListPaneHeight = 738;
	int HistoPaneWidth = 492;
	int HistoPaneHeight = 369;
	int ImagePaneWidth = 492;
	int ImagePaneHeight = 369;
	int AlphaPhiPaneWidth = 180;
	int AlphaPhiPaneHeight = 100;

	HTMLStream << "<html>" << endl;
        HTMLStream << "   <meta http-equiv='CACHE-CONTROL' content='NO-STORE'>\n"
		   //<< "   <meta http-equiv='PRAGMA' content='NO-CACHE'>\n"
		   //<< "   <meta http-equiv='EXPIRES' content='0'>\n"
		   << "   <meta name='KEYWORDS' content='CAFE,pattern recognition,fingerprint,weather event'>\n"
		   << "   <meta name='ROBOTS' content='NOFOLLOW'>" << endl;
	HTMLStream << "<body>" << endl;
	HTMLStream << "<p align='center'><b><font style='font-size: 12pt'>CAFE Status Viewer</font></b><br>" << endl;
	HTMLStream << "<i><font style='font-size: 10pt'>";


	for (set<string>::const_iterator ATimePeriod = TimePeriods.begin();
             ATimePeriod != TimePeriods.end();
             ATimePeriod++)
        {
        	HTMLStream << "<a target='EventPane' href='EventList_" << *ATimePeriod << ".html'>"
                           << *ATimePeriod << "</a>&nbsp;&nbsp;&nbsp;";
        }

	HTMLStream << "</p>\n";

		
	HTMLStream << "<table border='0' cellpadding='0' cellspacing='0' style='border-collapse: collapse' bordercolor='#FFFFFF' width='1200' id='AutoNumber1'>" << endl;
	
	HTMLStream << "<tr>" << endl;
        MakeEventPane(HTMLStream, EventPaneHeight, EventPaneWidth, 2, 1, "DefaultEventPane.html");
        MakeListPane(HTMLStream, ListPaneHeight, ListPaneWidth, 2, 1, "DefaultListPane.html");
	MakeImagePane(HTMLStream, ImagePaneHeight, ImagePaneWidth, 1, 2, "DefaultImagePane.html");
	HTMLStream << "</tr>" << endl;

	HTMLStream << "<tr>" << endl;
	MakeHistoPane(HTMLStream, HistoPaneHeight, HistoPaneWidth, 1, 1, "DefaultHistoPane.html");
	MakeAlphaPhiPane(HTMLStream, AlphaPhiPaneHeight, AlphaPhiPaneWidth, 1, 1, "DefaultAlphaPhiPane.html");
	HTMLStream << "</tr>" << endl;
	HTMLStream << "</table>" << endl;

	HTMLStream << "</body>";
	HTMLStream << "</html>";
	HTMLStream.close();
}
	
int main(int argc, char *argv[])
{
	vector<string> CommandArgs = ProcessFlatCommandLine(argc, argv);

	CmdOptions CAFEOptions;
	string WebLoc = "./";

        if (CAFEOptions.ParseCommandArgs(CommandArgs) != 0)
        {
                cerr << "ERROR: Invalid syntax..." << endl;
                PrintSyntax(CAFEOptions);
                return(8);
        }

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
		else if (CommandArgs[ArgIndex].find("--webloc=") == 0)
		{
			WebLoc = CommandArgs[ArgIndex].substr(9);
		}
                else
                {
                        cerr << "ERROR: Unknown option: " << CommandArgs[ArgIndex] << endl;
                        PrintHelp(CAFEOptions);
                        return(8);
                }
        }

	if (WebLoc.empty())
	{
		WebLoc = ".";
	}

	Configuration ConfigInfo(CAFEOptions.CAFEPath + '/' + CAFEOptions.ConfigFilename);             // loads configurations

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


	mysqlpp::Connection ServerLink;

        try
        {
		EstablishConnection(ServerLink, currState.GetServerName(), currState.GetCAFEUserName(), "", false);
        }
        catch (const exception& Err)
        {
                cerr << "ERROR: Connection failed: " << Err.what() << endl;
		ServerLink.close();
                return(3);
        }
	catch (const string& ErrStr)
        {
                cerr << "ERROR: Connection failed: " << ErrStr << endl;
                ServerLink.close();
                return(3);
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught..." << endl;
                ServerLink.close();
                return(4);
        }


	const string HTMLName = WebLoc + "/index.html";

	try
        {
		MakeMainFile(HTMLName, currState.TimePeriod_Names());

		for (currState.TimePeriods_Begin(); currState.TimePeriods_HasNext(); currState.TimePeriods_Next())
		{
			const string Database = currState.Untrained_Name();
			const string ClustDatabase = currState.Trained_Name();
			const string eventListName = "EventList_" + currState.TimePeriod_Name() + ".html";

                        if (!ServerLink.select_db(ClustDatabase))
                        {
                                throw("Could not select the database: " + ClustDatabase + "\nMySQL message: " + ServerLink.error());
                        }


                        mysqlpp::Query TheQuery = MakeLoader_MemberCnt(ServerLink);

			const string FullEventListName = WebLoc + '/' + eventListName;

			ofstream EventStream(FullEventListName.c_str());

			if (!EventStream.is_open())
	        	{
        	        	throw("Could not open this file: " + FullEventListName);
	        	}

			EventStream << "<html><body>" << endl;
			EventStream << "<p align='left'><b><u><font style='font-size: 13pt'>" << endl;
		
       			for (currState.EventTypes_Begin(); currState.EventTypes_HasNext(); currState.EventTypes_Next())
	        	{
				TheQuery.def["table"] = currState.EventType_Name();

				const string CompareListStem = "CompareList_" + currState.EventType_Name() + '_' 
							       + currState.TimePeriod_Name() + ".html";
				const string CompareListName = WebLoc + '/' + CompareListStem;

				EventStream << "<a target='ListPane' href='" << CompareListStem << "'>" 
					    << currState.EventType_Name() << "</a><br>" << endl;

				ofstream CompareStream(CompareListName.c_str());

		                if (!CompareStream.is_open())
        		        {
                		        throw("Could not open this file: " + CompareListName);
        	        	}

				MakeHeaders(CompareStream, currState.TimePeriod_Name());
				CompareStream << "<p align='left'><b><font style='font-size: 13pt'>" 
					      << currState.EventType_Name() << "</font></b>" << endl;
				CompareStream << "<font style='font-size: 10pt'>" << endl;
			
				for (currState.EventVars_Begin(); currState.EventVars_HasNext(); currState.EventVars_Next())
				{
					for (currState.EventLevels_Begin(); currState.EventLevels_HasNext(); currState.EventLevels_Next())
        	                        {
						CompareStream << "<p>" << endl;

        			               	for (currState.Extrema_Begin(); currState.Extrema_HasNext(); currState.Extrema_Next())
	        	        	        {
	       	                        		const string FieldName = currState.FieldExtremum_Name();
							const string MapPlotName = currState.EventType_Name() + '_' + FieldName + "_MapPlot.jpg";
							const string HistPlotName = currState.EventType_Name() + '_' + FieldName + "_Hist.jpg";
							const string FieldMeasureName = currState.EventType_Name() + '_' + FieldName + ".FieldMeasure";

							CompareStream << "<b>";
							CompareStream << "<li>&nbsp;&nbsp;&nbsp;<a target='ImagePane' href='"
								      << "Images/" << Database << '/' << MapPlotName << "'"
								      << " onClick=\"top.AlphaPhiPane.location='"
								      << "Images/" << ClustDatabase << '/' << FieldMeasureName << "';"
								      << "top.HistoPane.location='"
								      << "Images/" << Database <<  '/' << HistPlotName << "';"
								      << "\">Unclustered " << FieldName << "</a></li>" << endl;
							CompareStream << "</b>";

							const size_t MemberCount = LoadMemberCnt(TheQuery, FieldName, currState.EventType_Name());
							if (MemberCount > 0)
							// then there must have been a cluster made
							{
								CompareStream << "<b>";
								CompareStream << "<li>&nbsp;&nbsp;&nbsp;<a target='ImagePane' href='"
									      << "Images/" + ClustDatabase << '/' << MapPlotName << "'"
									      << " onClick=\"top.AlphaPhiPane.location='"
									      << "Images/" << ClustDatabase << '/' << FieldMeasureName << "';"
									      << "top.HistoPane.location='"
									      << "Images/" << ClustDatabase << '/' << HistPlotName << "';"
									      << "\">Clustered &nbsp;&nbsp;&nbsp;&nbsp;" << FieldName
									      << "</a></li>" << endl;
								CompareStream << "</b>";
							}
							else
							{
								CompareStream << "<i>";
								CompareStream << "<li>&nbsp;&nbsp;&nbsp;Clustered &nbsp;&nbsp;&nbsp;&nbsp;" 
									      << FieldName << "</li>";
								CompareStream << "</i>" << endl;
							}
						}// end extrema loop
					
					}// end label loop
					CompareStream << "</p>" << endl;
				}// end CAFEVarName loop

				CompareStream << "</font>" << endl;
				CompareStream << "</p><p>&nbsp;</p>" << endl;
				CompareStream << "</body>" << endl;
		                CompareStream << "</html>" << endl;
				CompareStream.close();
			}// end EventType loop

			EventStream << "</font></p>" << endl;
			EventStream << "</body>" << endl;
			EventStream << "</html>" << endl;

			EventStream.close();
		}// end databse loop
	}
	catch (const exception &Err)
        {
                cerr << "ERROR: Exception was thrown: " << Err.what() << endl;
                ServerLink.close();
                return(5);
        }
        catch (const string &ErrStr)
        {
                cerr << "ERROR: " << ErrStr << endl;
                ServerLink.close();
                return(6);
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught..." << endl;
                ServerLink.close();
                return(7);
        }


	ServerLink.close();
	cout << "\n\tAll Done!\n";
	return(0);
}

