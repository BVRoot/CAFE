#ifndef _CAFE_SQLUTLY_C
#define _CAFE_SQLUTLY_C

using namespace std;

#include <vector>
#include <string>
#include <mysql++/mysql++.h>

#define MYSQLPP_SSQLS_NO_STATICS       // makes sure that the SSQL structs are only declared, not defined.
#include "Utils/CAFE_SQLStructs.h"			// for LonLatAnom and LonLatAnomDate

#include <cmath>
#include <ctime>
#include <unistd.h>				// for getpass()

#include <StrUtly.h>
#include <TimeUtly.h>				// for GiveTime()

#include "Config/Configuration.h"
#include "Utils/CAFE_CmdLine.h"

#include "SPAnalysis/ClusterBoard.h"
#include "SPAnalysis/BoardConvertor.h"
#include "SPAnalysis/BoardType.h"

#include "Utils/CAFE_SQLUtly.h"



void EstablishConnection(mysqlpp::Connection &ConnectLink, const string &HostName, 
			 const string &UserName, const string &DatabaseName, const bool NeedPass)
{
	if (NeedPass)
	{
		char* ThePass = getpass(("Enter MySQL password for user " + UserName + " on host " + HostName + ": ").c_str());

		if (ThePass == NULL)
        	{
                	throw((string) "PASSWORD ERROR!");
	        }

		try
        	{
                	ConnectLink.connect(DatabaseName.c_str(), HostName.c_str(), UserName.c_str(), ThePass);
	        }
        	catch (...)
	        {
        	        while (*ThePass != '\0')
                	{
                        	*ThePass++ = '\0';
	                }

        	        throw;
        	}

	        while (*ThePass != '\0')
        	{
                	*ThePass++ = '\0';
        	}
	}
	else
	{
		ConnectLink.connect(DatabaseName.c_str(), HostName.c_str(), UserName.c_str(), "");
	}
}


//-------------------------------------------------------------------------------------------------------------------------------------
//*************************************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------

time_t DateTimeToTime_t(const mysqlpp::DateTime &SQLTime)
// warning, it will assume that the time being given is the local time.
{
	struct tm ThisTime;
        ThisTime.tm_sec = SQLTime.second;
        ThisTime.tm_min = SQLTime.minute;
        ThisTime.tm_hour = SQLTime.hour;
        ThisTime.tm_mday = SQLTime.day;
        ThisTime.tm_mon = ((int) SQLTime.month - 1);
        ThisTime.tm_year = (SQLTime.year - 1900);


	return(mktime(&ThisTime));
}

mysqlpp::DateTime Time_tToDateTime(const time_t &TheTime)
// warning, this uses localtime_r(), so be careful!
{
	mysqlpp::DateTime SQLTime(TheTime);
/*
	struct tm ThisTime;
	localtime_r(&TheTime, &ThisTime);

	SQLTime.second = ThisTime.tm_sec;
	SQLTime.minute = ThisTime.tm_min;
	SQLTime.hour = ThisTime.tm_hour;
	SQLTime.day = ThisTime.tm_mday;
	SQLTime.month = ThisTime.tm_mon + 1;
	SQLTime.year = ThisTime.tm_year + 1900;
*/
	return(SQLTime);
}

string DateTimeToStr(const mysqlpp::DateTime &SQLTime)
{
	char DateStr[20];
	memset(DateStr, '\0', 20);
	if (snprintf(DateStr, 20, "%0.4i-%0.2i-%0.2i %0.2i:%0.2i:%0.2i",
		    SQLTime.year,
		    (int) SQLTime.month,
		    (int) SQLTime.day,
		    (int) SQLTime.hour,
		    (int) SQLTime.minute,
		    (int) SQLTime.second) < 0)
	{
		cerr << "WARNING: Bad conversion of time: " << SQLTime << endl;
		return("0000-00-00 00:00:00");
	}

	return((string) DateStr);
}


// Temporary code...
void SplitIntoVects(const vector <LonLatAnomDate> &TheMembers, 
		    vector <double> &Lons, vector <double> &Lats, vector <double> &Anoms, vector <time_t> &DateTimes)
{
	Lons.resize(TheMembers.size());
	Lats.resize(TheMembers.size());
	Anoms.resize(TheMembers.size());
	DateTimes.resize(TheMembers.size());

	vector<double>::iterator ALon( Lons.begin() ), ALat( Lats.begin() ), AnAnom( Anoms.begin() );
	vector<time_t>::iterator ADate( DateTimes.begin() );

	for (vector<LonLatAnomDate>::const_iterator AMember( TheMembers.begin() ); AMember != TheMembers.end(); 
	     AMember++, ALon++, ALat++, AnAnom++, ADate++)
	{
		*ALon = AMember->Lon;
		*ALat = AMember->Lat;
		*AnAnom = AMember->StdAnom;
		*ADate = DateTimeToTime_t(AMember->DateInfo);
	}
}

// Temporary code
void SplitIntoVects(const vector <LonLatAnom> &TheMembers,
                    vector <double> &Lons, vector <double> &Lats, vector <double> &Anoms)
{
        Lons.resize(TheMembers.size());
        Lats.resize(TheMembers.size());
        Anoms.resize(TheMembers.size());

        vector<double>::iterator ALon( Lons.begin() ), ALat( Lats.begin() ), AnAnom( Anoms.begin() );

        for (vector<LonLatAnom>::const_iterator AMember( TheMembers.begin() ); AMember != TheMembers.end();
             AMember++, ALon++, ALat++, AnAnom++)
        {
                *ALon = AMember->Lon;
                *ALat = AMember->Lat;
                *AnAnom = AMember->StdAnom;
        }
}

// Temporary code...
vector <mysqlpp::DateTime> SplitIntoTime(const vector <LonLatAnomDate> &TheMembers)
{
	vector <mysqlpp::DateTime> TheTimes(TheMembers.size());

	vector<LonLatAnomDate>::const_iterator AMember( TheMembers.begin() );
	for (vector<mysqlpp::DateTime>::iterator ATime( TheTimes.begin() ); ATime != TheTimes.end(); ATime++, AMember++)
	{
		*ATime = AMember->DateInfo;
	}

	return(TheTimes);
}



mysqlpp::Query MakeLoader_LonLatAnoms(mysqlpp::Connection &TheConnection)
{
	mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
		TheQuery << "SELECT %0_Lon AS Lon, %0_Lat AS Lat, %0_StdAnom AS StdAnom FROM %1:table WHERE %0_Lon IS NOT NULL";
		TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
	{
		cerr << "ERROR: Exception caught: " << Err.what() << endl;
		cerr << "query: " << TheQuery.preview() << endl;
		throw;
	}
	catch (...)
	{
		cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
	}

	return(TheQuery);
}

vector <LonLatAnom> LoadLonLatAnoms(mysqlpp::Query &TheQuery, const string &FieldStem)
{
//        TheQuery << "select " << FieldStem << "_Lon as Lon, " << FieldStem << "_Lat as Lat, " << FieldStem << "_StdAnom as StdAnom"
//                 << " from " << EventTypeName << " where " << FieldStem << "_Lon IS NOT NULL";
        vector <LonLatAnom> TheResults;

	try
	{

        TheQuery.storein(TheResults, FieldStem);

	if (!TheQuery.success())
	{
		throw("Problem loading lonlats for field " + FieldStem + " and event type " + TheQuery.def["table"] 
		      + "\nMySQL message: " + TheQuery.error());
	}

	}
	catch (...)
	{
		cerr << "ERROR: Problem loading lonlats for field " + FieldStem + " and event type " + TheQuery.def["table"]
			+ "\nTheQuery: " << TheQuery.preview(FieldStem) << endl;
		throw;
	}

	return(TheResults);
}


mysqlpp::Query MakeLoader_LonLatAnomDates(mysqlpp::Connection &TheConnection)
{

	mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
        	TheQuery << "SELECT %0_Lon AS Lon, %0_Lat AS Lat, %0_StdAnom AS StdAnom, DateInfo FROM %1:table WHERE %0_Lon IS NOT NULL";
	        TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

	return(TheQuery);
}


vector <LonLatAnomDate> LoadLonLatAnomDates(mysqlpp::Query &TheQuery, const string &FieldStem)
{
//        TheQuery << "select " << FieldStem << "_Lon as Lon, " << FieldStem << "_Lat as Lat, " << FieldStem << "_StdAnom as StdAnom, "
//		 << "DateInfo from " << EventTypeName << " where " << FieldStem << "_Lon IS NOT NULL";

        vector <LonLatAnomDate> TheResults;

        TheQuery.storein(TheResults, FieldStem);

	if (!TheQuery.success())
	{
		throw("Problem loading lonlats for field " + FieldStem + " and event type " + TheQuery.def["table"] + "\nMySQL message: " + TheQuery.error());
	}

        return(TheResults);
}

mysqlpp::Query MakeLoader_MemberCnt(mysqlpp::Connection &TheConnection)
{
	
        mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
	        TheQuery << "SELECT COUNT(DateInfo) AS MemberCount FROM %1:table WHERE %0_Lon IS NOT NULL";
        	TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

	return(TheQuery);
}


size_t LoadMemberCnt(mysqlpp::Query &TheQuery, const string &FieldStem, const string &TableName)
// FieldStem refers to the stem with the extremum name.
{
//        TheQuery << "SELECT COUNT(DateInfo) AS MemberCount FROM " << TableName
//		 << " WHERE " << FieldStem << "_Lon IS NOT NULL";
//	TheQuery.reset();
	try
	{
//        mysqlpp::Result TheResult( TheQuery.store(FieldStem) );
//	TheQuery << "SELECT COUNT(DateInfo) AS MemberCount FROM " << TableName
//		 << " WHERE " << FieldStem << "_Lon IS NOT NULL";

	mysqlpp::Result TheResult = TheQuery.store(FieldStem, TableName);

        if (!TheQuery.success())
        {
                throw("Problem getting member count for field " + FieldStem + " and event type " + TheQuery.def["table"] + "\nMySQL message: " + TheQuery.error());
        }

        if (TheResult)
        {
                mysqlpp::Result::iterator ARow( TheResult.begin() );
                return((*ARow)["MemberCount"]);
        }

        return(0);
	}
	catch(...)
	{
		cerr << "ERROR: Problem with query: " << TheQuery.preview(FieldStem, TableName) << endl;
		throw;
	}
}

mysqlpp::Query MakeLoader_EventCnt(mysqlpp::Connection &TheConnection)
{
	
        mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
	        TheQuery << "SELECT COUNT(DateInfo) AS EventCount FROM %0:table";
        	TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

	return(TheQuery);

}


size_t LoadEventCnt(mysqlpp::Query &TheQuery, const string &TableName)
{
	TheQuery.reset();
	TheQuery << "SELECT COUNT(DateInfo) AS EventCount FROM " << mysqlpp::escape << TableName;
        mysqlpp::Result TheResult = TheQuery.store();

	if (!TheQuery.success())
	{
		throw("Problem getting event count for event type " + TableName + "\nMySQL message: " + TheQuery.error());
	}

        if (TheResult)
        {
	        mysqlpp::Result::iterator ARow( TheResult.begin() );
                return((*ARow)["EventCount"]);
        }

	return(0);
}

/*
mysqlpp::Query MakeLoader_EventDateTimes(mysqlpp::Connection &TheConnection)
{

	mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
        	TheQuery << "SELECT DateInfo FROM %0:table";
	        TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }


        return(TheQuery);
}
*/

vector <time_t> LoadEventDateTimes(mysqlpp::Query &TheQuery, const string &TableName)
{
        TheQuery << "SELECT DateInfo FROM " << mysqlpp::escape << TableName;

//	mysqlpp::Result TheResult( TheQuery.store(TableName) );

	mysqlpp::Result TheResult = TheQuery.store();

        if (!TheQuery.success())
        {
		throw("Problem extracting event date times for event type " + TableName + "\nMySQL message: " + TheQuery.error());
        }

	if (TheResult)
        {
		vector <time_t> EventTimes( TheResult.size() );
		vector<time_t>::iterator ATime = EventTimes.begin();
                for (mysqlpp::Result::iterator ARow( TheResult.begin() ); ARow != TheResult.end(); ARow++, ATime++)
		{
			*ATime = DateTimeToTime_t( (*ARow)["DateInfo"] );
		}
		
		return(EventTimes);
        }
	else
	{
		return(vector<time_t>(0));
	}
}






void InsertEvent(const time_t &EventDateTime, const double &EventLon, const double &EventLat, const vector <double> &TheValues,
		 mysqlpp::Query &TheQuery, const string &TableName)
// assumes that the values in TheValues correspond to the table's column in the SAME ORDER as they were created.
{
	TheQuery << "INSERT IGNORE INTO " << TableName << " VALUES('" << Time_tToDateTime(EventDateTime) << "'," << EventLon << ',' << EventLat
		 << ',' << GiveDelimitedList(DoubleToStr(TheValues), ',') << ')';

	TheQuery.execute();

	if (!TheQuery.success())
	{
		throw("Problem doing event insertion into the database for event type " + TableName + "\nMySQL message: " + TheQuery.error());
	}

	TheQuery.reset();
}

void InsertEvent(const time_t &EventDateTime, const double &EventLon, const double &EventLat, mysqlpp::Query &TheQuery, const string &TableName)
{
	TheQuery << "INSERT IGNORE INTO " << TableName << " (DateInfo, Event_Lon, Event_Lat) "
		 << " VALUES('" << Time_tToDateTime(EventDateTime) << "'," << EventLon << ',' << EventLat << ')';

	TheQuery.execute();

	if (!TheQuery.success())
	{
		throw("Problem doing event insertion into the database for event type " + TableName + "\nMySQL message: " + TheQuery.error());
	}

	TheQuery.reset();
}



mysqlpp::Query MakeSaver_LonLatAnomDate(mysqlpp::Connection &TheConnection)
{
	
        mysqlpp::Query TheQuery = TheConnection.query();
	try
	{
	        TheQuery << "UPDATE %5:table SET %0_Lon=%1, %0_Lat=%2, %0_StdAnom=%3 WHERE DateInfo = %4q LIMIT 1";
		TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

	return(TheQuery);
}


void SaveLonLatAnom(const double &Lon, const double &Lat, const double &Anom, const time_t ADateTime,
                     mysqlpp::Query &TheQuery, const string &FieldStem)
// DoubleToStr() used for now so that NaNs are converted to '\N' for mysql
{
//	TheQuery << "UPDATE " << EventTypeName << " SET "
//                 << FieldStem << "_Lon=" << DoubleToStr(Lon) << ", "
//                 << FieldStem << "_Lat=" << DoubleToStr(Lat) << ", "
//                 << FieldStem << "_StdAnom=" << DoubleToStr(Anom)
//                 << " WHERE DateInfo = '" << Time_tToDateTime(ADateTime) << "' LIMIT 1";

	TheQuery.execute(FieldStem, DoubleToStr(Lon), DoubleToStr(Lat), DoubleToStr(Anom), GiveTime(ADateTime));

        if (!TheQuery.success())
        {
                throw("Could not save the lon/lat/anom value to the database for date: " + GiveTime(ADateTime) + " for field " + FieldStem
		      + " for event type " + TheQuery.def["table"] + "\nMySQL message: " + TheQuery.error());
        }
}


void SaveLonLatAnoms(const vector <double> &Lons, const vector <double> &Lats, const vector <double> &Anoms, const vector <mysqlpp::DateTime> &DateTimes, 
		     mysqlpp::Query &TheQuery, const string &FieldStem)
{
	if (Lons.size() != Lats.size() || Lons.size() != Anoms.size() || Lons.size() != DateTimes.size())
	{
		throw((string) "SaveLonLatAnoms(): These are not parallel vectors...");
	}

	vector<mysqlpp::DateTime>::const_iterator ATime( DateTimes.begin() );
	for (vector<double>::const_iterator ALon( Lons.begin() ), ALat( Lats.begin() ), AnAnom( Anoms.begin() ); ALon != Lons.end();
	     ALon++, ALat++, AnAnom++, ATime++)
	{
		TheQuery.execute(FieldStem, DoubleToStr(*ALon), DoubleToStr(*ALat), DoubleToStr(*AnAnom), DateTimeToStr(*ATime));
	}

	if (!TheQuery.success())
	{
		throw("Problem saving the lon/lat/anoms for field " + FieldStem + " and for event type " + TheQuery.def["table"]
		      + "\nMySQL message: " + TheQuery.error());
	}
}

void SaveBoardToDatabase(const ClusterBoard &TheBoard, mysqlpp::Query &TheQuery, const string &FieldStem,
		         const BoardConvertor &ProjectionInfo)
// Note, you can't tell right now if the table was actually updated or not.
// This should probably be fixed, somehow.
{
//        TheQuery << "UPDATE " << TableName << " SET "
//                 << FieldStem << "_Lon=%0,"
//                 << FieldStem << "_Lat=%1,"
//                 << FieldStem << "_StdAnom=%2"
//                 << " WHERE DateInfo = %3q LIMIT 1";
//        TheQuery.parse();
	try
	{
        	for (size_t XLoc = 0; XLoc < ProjectionInfo.Xsize(); XLoc++)
        	{
                	for (size_t YLoc = 0; YLoc < ProjectionInfo.Ysize(); YLoc++)
	                {
        	 //		double SphericalLon, SphericalLat;
                //        	ProjectionInfo.GridToSpherical(XLoc, YLoc, SphericalLon, SphericalLat);
				const vector <LonLatAnomDate> TheMembers = TheBoard.ReturnMembers(XLoc, YLoc).ReturnMembers();
	                        for (vector<LonLatAnomDate>::const_iterator AMember = TheMembers.begin(); 
				     AMember != TheMembers.end();
				     AMember++)
        	                {
					TheQuery.execute(FieldStem, AMember->Lon, AMember->Lat, AMember->StdAnom, DateTimeToStr(AMember->DateInfo));
//					cout << SphericalLon << ", " << SphericalLat << ", " << TempGridPoint.GiveMemberValue(MemberIndex)
//					     << ", " << GiveTime(TempGridPoint.GiveMemberDate(MemberIndex)) << "\n";
				}
			}
		}
	}
	catch (...)
	{
		cerr << "ERROR: Problem saving the lon/lat/anoms for field " << FieldStem << ", event type " << TheQuery.def["table"] << endl;
		throw;
	}

	if (!TheQuery.success())
	{
		throw("Problem saving the lon/lat/anoms for field " + FieldStem + " and for event type " + TheQuery.def["table"]
		      + "\nMySQL message: " + TheQuery.error());
        }
}

vector <mysqlpp::DateTime> GiveClusteredDates(const ClusterBoard &TheBoard, const BoardConvertor &ProjectionInfo)
// Temporary until I fix and reorganize the clustering algorithm.
// The dates will be in ascending order.
{
	vector <mysqlpp::DateTime> TheDates(0);
        for (size_t XLoc = 0; XLoc < ProjectionInfo.Xsize(); XLoc++)
        {
                for (size_t YLoc = 0; YLoc < ProjectionInfo.Ysize(); YLoc++)
                {
                        const vector<LonLatAnomDate> TheMembers = TheBoard.ReturnMembers(XLoc, YLoc).ReturnMembers();

			for (size_t MemberIndex = 0; MemberIndex < TheMembers.size(); MemberIndex++)
                        {
				TheDates.insert(lower_bound(TheDates.begin(), TheDates.end(), TheMembers[MemberIndex].DateInfo), 
						TheMembers[MemberIndex].DateInfo);
			}
		}
	}

	return(TheDates);
}


vector <string> GiveTableNames(mysqlpp::Query &TheQuery, const string &Database)
// Returns the names of the Tables available in the database in alphabetical order.
// Returns an empty array if there are no databases selected.
// Errors will be thrown if the database is not the same database that the query is connected to.
{
	TheQuery << "SHOW TABLES";
        mysqlpp::Result TableResult = TheQuery.store();

        vector <string> TableNames(0);

        for (mysqlpp::Result::iterator ARow = TableResult.begin(); ARow != TableResult.end(); ARow++)
        {
	        string EventName( (*ARow)[("Tables_in_" + Database).c_str()].get_string() );
                TableNames.insert(lower_bound(TableNames.begin(), TableNames.end(), EventName), EventName);
        }

	return(TableNames);
}

bool DropTables(mysqlpp::Query &TheQuery, const vector <string> &TableNames)
{
	if (TableNames.empty())
        {
                return(true);
        }

	if (!TheQuery.exec("DROP TABLE IF EXISTS " + GiveDelimitedList(TableNames, ',')))
	{
		return(false);
	}

// A bit of a kludge for now.
	string TableList = *TableNames.begin() + "_FieldMeas";
	for (vector<string>::const_iterator ATable = TableNames.begin() + 1; ATable != TableNames.end(); ATable++)
	{
		TableList += "," + *ATable + "_FieldMeas";
	}

	return(TheQuery.exec("DROP TABLE IF EXISTS " + TableList));
}

bool ClearTable(mysqlpp::Query &TheQuery, const string &TableName)
{
	return(TheQuery.exec("DELETE FROM " + TableName));
}

bool UpdateTable(const vector <double> &TheValues, const vector <string> &ColumnNames, const time_t &ADateTime, 
		 mysqlpp::Query &TheQuery, const string &TableName)
{
	if (TheValues.empty())
	{
		return(true);
	}

	TheQuery << "UPDATE " << TableName << " SET " << ColumnNames[0] << '=' << DoubleToStr(TheValues[0]);
	for (size_t Index = 1; Index < TheValues.size(); Index++)
	{
		TheQuery << ", " << ColumnNames[Index] << '=' << DoubleToStr(TheValues[Index]);
	}

	TheQuery << " WHERE DateInfo = '" << Time_tToDateTime(ADateTime) << "'";

	TheQuery.execute();
	return(TheQuery.success());
}


void CreateFieldMeasureTable(mysqlpp::Query &TheQuery, const string &EventTypeName, const Configuration &ConfigInfo, const CmdOptions &CAFEOptions)
{
	size_t MaxSize = 0;
	const vector <string> VarNames = CAFEOptions.GiveCAFEVarsToDo(ConfigInfo, EventTypeName);
	vector <string> FieldNames(0);
	
        for (vector<string>::const_iterator AVarName = VarNames.begin(); AVarName != VarNames.end(); AVarName++)
        {
                const vector <string> CAFELabels = CAFEOptions.GiveLabelsToDo(ConfigInfo, EventTypeName, *AVarName);

                for (vector<string>::const_iterator ALabel = CAFELabels.begin(); ALabel != CAFELabels.end(); ALabel++)
                {
                        for (size_t PeakValIndex = 0; PeakValIndex < ConfigInfo.ExtremaCount(); PeakValIndex++)
                        {
				FieldNames.push_back(*ALabel + '_' + ConfigInfo.GiveExtremaName(PeakValIndex));
				if (FieldNames.back().size() > MaxSize)
				{
					MaxSize = FieldNames.back().size();
				}
			}
		}
	}
			


	TheQuery << "create table IF NOT EXISTS " << mysqlpp::escape << EventTypeName << "_FieldMeas (FieldName VARCHAR(" 
		 << MaxSize << ") NOT NULL PRIMARY KEY,"
		 << " Alpha float(8, 5), Phi float(8, 5), Gamma_Max float(8, 5), Chi_Max float(8, 5))";

	TheQuery.execute();
	if (!TheQuery.success())
	{
		throw("Couldn't create the field measure table for event type " + EventTypeName + "\nMySQL message: " + TheQuery.error());
	}

	TheQuery << "INSERT IGNORE INTO " << mysqlpp::escape << EventTypeName << "_FieldMeas (FieldName) VALUES('" 
		 << GiveDelimitedList(FieldNames, "'),('") << "')";
	TheQuery.execute();

	if (!TheQuery.success())
	{
		throw("Couldn't insert the field names into the field measure table for event type " + EventTypeName 
		      + "\nMySQL message: " + TheQuery.error());
	}
}


mysqlpp::Query MakeSaver_AlphaPhiValues(mysqlpp::Connection &TheConnection)
{
	mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
	        TheQuery << "UPDATE %3:table:_FieldMeas SET Alpha=%1, Phi=%2 WHERE FieldName = %0q LIMIT 1";
        	TheQuery.parse();

	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

	
	return(TheQuery);
}


void SaveAlphaPhiValues(mysqlpp::Query &TheQuery, const string &FieldName,
			const double &AlphaVal, const double &PhiVal)
{
//	TheQuery << "UPDATE " << EventTypeName << "_FieldMeas SET Alpha=" << DoubleToStr(AlphaVal) << ", Phi=" << DoubleToStr(PhiVal)
//		 << " WHERE FieldName = '" << FieldName << "' LIMIT 1";
	try
	{
		TheQuery.execute(FieldName, AlphaVal, PhiVal);
	}
	catch (...)
	{
		cerr << "ERROR: Problem saving alpha/phi values for field " << FieldName << ", event type " << TheQuery.def["table"] << endl;
		throw;
	}

	if(!TheQuery.success())
	{
		throw("Could not save alpha/phi values for field " + FieldName + " for event type " + TheQuery.def["table"] 
		      + "\nMySQL message: " + TheQuery.error());
	}

//	TheQuery.reset();
}


mysqlpp::Query MakeLoader_AlphaPhiValues(mysqlpp::Connection &TheConnection)
{
        mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
	        TheQuery << "SELECT Alpha, Phi FROM %1:table:_FieldMeas WHERE FieldName = %0q LIMIT 1";
        	TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

	return(TheQuery);
}


void LoadAlphaPhiValues(mysqlpp::Query &TheQuery, const string &FieldName,
                        double &AlphaVal, double &PhiVal)
{
//        TheQuery << "SELECT Alpha, Phi FROM " << EventTypeName << "_FieldMeas"
//                 << " WHERE FieldName = '" << FieldName << "' LIMIT 1";
        mysqlpp::Result TheResult( TheQuery.store(FieldName) );

        if (!TheQuery.success())
        {
                throw("Problem extracting Alpha/Phi values for field " + FieldName + " for event type " + TheQuery.def["table"]
		      + "\nMySQL message: " + TheQuery.error());
        }

        if (TheResult)
        {
                mysqlpp::Result::iterator ARow( TheResult.begin() );
                AlphaVal = (*ARow)["Alpha"];
		PhiVal = (*ARow)["Phi"];
        }
	else
	{
		AlphaVal = PhiVal = nan("nan");
	}

//	TheQuery.reset();
}


mysqlpp::Query MakeLoader_GammaChiMaxValues(mysqlpp::Connection &TheConnection)
{
        mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
        	TheQuery << "SELECT Gamma_Max, Chi_Max FROM %1:table:_FieldMeas WHERE FieldName = %0q LIMIT 1";
	        TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

        return(TheQuery);
}


void LoadGammaChiMaxValues(mysqlpp::Query &TheQuery, const string &FieldName,
			   double &GammaMax, double &ChiMax)
{
//        TheQuery << "SELECT Gamma_Max, Chi_Max FROM " << EventTypeName << "_FieldMeas"
//                 << " WHERE FieldName = '" << FieldName << "' LIMIT 1";
        mysqlpp::Result TheResult( TheQuery.store(FieldName) );

        if (!TheQuery.success())
        {
	        throw("Problem extracting gamma/chi max values for field " + FieldName + " for event type " + TheQuery.def["table"]
                      + "\nMySQL message: " + TheQuery.error());
        }

        if (TheResult)
        {
                mysqlpp::Result::iterator ARow( TheResult.begin() );
                GammaMax = (*ARow)["Gamma_Max"];
                ChiMax = (*ARow)["Chi_Max"];
        }
        else
        {
                GammaMax = ChiMax = nan("nan");
        }
}


mysqlpp::Query MakeLoader_FieldMeasValues(mysqlpp::Connection &TheConnection)
{
        mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
	        TheQuery << "SELECT Alpha, Phi, Gamma_Max, Chi_Max FROM %1:table:_FieldMeas WHERE FieldName = %0q LIMIT 1";
        	TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

        return(TheQuery);
}



void LoadFieldMeasValues(mysqlpp::Query &TheQuery, const string &FieldName, const string &TableName,
                         double &AlphaVal, double &PhiVal, double &GammaMax, double &ChiMax)
{
//        TheQuery << "SELECT Alpha, Phi, Gamma_Max, Chi_Max FROM " << EventTypeName << "_FieldMeas"
//                 << " WHERE FieldName = '" << FieldName << "' LIMIT 1";

	try
	{
        mysqlpp::Result TheResult( TheQuery.store(FieldName, TableName) );

        if (!TheQuery.success())
        {
                throw("Problem extracting field measure values for field " + FieldName + " for event type " + TheQuery.def["table"]
                      + "\nMySQL message: " + TheQuery.error());
        }

        if (TheResult)
        {
                mysqlpp::Result::iterator ARow( TheResult.begin() );
		AlphaVal = ((*ARow)["Alpha"].get_string() == "NULL" ? nan("nan") : (*ARow)["Alpha"]);
		PhiVal = ((*ARow)["Phi"].get_string() == "NULL" ? nan("nan") : (*ARow)["Phi"]);
                GammaMax = ((*ARow)["Gamma_Max"].get_string() == "NULL" ? nan("nan") : (*ARow)["Gamma_Max"]);
                ChiMax = ((*ARow)["Chi_Max"].get_string() == "NULL" ? nan("nan") : (*ARow)["Chi_Max"]);
        }
        else
        {
		AlphaVal = PhiVal = GammaMax = ChiMax = nan("nan");
        }
	}
	catch (...)
	{
		cerr << "ERROR: Problem extracting field measure values for field " + FieldName + " for event type " + TheQuery.def["table"]
		     << "\nMySQL query: " << TheQuery.preview(FieldName) << endl;
		throw;
	}

//	TheQuery.reset();
}




mysqlpp::Query MakeSaver_GammaChiMaxValues(mysqlpp::Connection &TheConnection)
{
	mysqlpp::Query TheQuery = TheConnection.query();

	try
	{
	        TheQuery << "UPDATE %3:table:_FieldMeas SET Gamma_Max=%1, Chi_Max=%2 WHERE FieldName = %0q LIMIT 1";
        	TheQuery.parse();
	}
	catch (mysqlpp::Exception &Err)
        {
                cerr << "ERROR: Exception caught: " << Err.what() << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }
        catch (...)
        {
                cerr << "ERROR: Unknown exception caught." << endl;
                cerr << "query: " << TheQuery.preview() << endl;
                throw;
        }

        return(TheQuery);
}

void SaveGammaChiMaxValues(mysqlpp::Query &TheQuery, const string &FieldName,
                        const double &GammaMax, const double &ChiMax)
{
//        TheQuery << "UPDATE " << EventTypeName << "_FieldMeas SET Gamma_Max=" << DoubleToStr(GammaMax) << ", Chi_Max=" << DoubleToStr(ChiMax)
//                 << " WHERE FieldName = '" << FieldName << "' LIMIT 1";

	TheQuery.execute(FieldName, GammaMax, ChiMax);

	if (!TheQuery.success())
	{
		throw("Couldn't save the gamma/chi max values for field " + FieldName + " for event type " + TheQuery.def["table"]
		      + "\nMySQL message: " + TheQuery.error());
	}
}


void CreateTable(mysqlpp::Query &TheQuery, const string &EventTypeName, const Configuration &ConfigInfo, const CmdOptions &CAFEOptions)
{
	vector <string> DataNames(3);
        DataNames[0] = "StdAnom float(8, 6)";
        DataNames[1] = "Lon float(7, 3)";
        DataNames[2] = "Lat float(7, 3)";

	TheQuery << "CREATE TABLE " << EventTypeName << "(DateInfo datetime NOT NULL PRIMARY KEY,"
		 << " Event_Lon float(6, 3) NOT NULL, Event_Lat float(6, 3) NOT NULL";

	const vector <string> VarNames = CAFEOptions.GiveCAFEVarsToDo(ConfigInfo, EventTypeName);
        for (vector<string>::const_iterator AVarName = VarNames.begin(); AVarName != VarNames.end(); AVarName++)
        {
	        const vector <string> CAFELabels = CAFEOptions.GiveLabelsToDo(ConfigInfo, EventTypeName, *AVarName);

                for (vector<string>::const_iterator ALabel = CAFELabels.begin(); ALabel != CAFELabels.end(); ALabel++)
		{
	                for (size_t PeakValIndex = 0; PeakValIndex < ConfigInfo.ExtremaCount(); PeakValIndex++)
                        {
	                        string TypeName = *ALabel + '_' + ConfigInfo.GiveExtremaName(PeakValIndex);
                                TheQuery << ',' << TypeName + '_' + DataNames.at(0);

                                for (size_t DataIndex = 1; DataIndex < DataNames.size(); DataIndex++)
                                {
        	                        TheQuery << ", " << TypeName + '_' + DataNames[DataIndex];
                                }
                        }// end of extremum loop
		}// end of Label loop
	}// end of CAFEVar loop

	TheQuery << ")";
	TheQuery.execute();

        if (!TheQuery.success())
	{
		throw("Couldn't create the table for event type " + EventTypeName + "\nMySQL message: " + TheQuery.error());
	}
}

#endif