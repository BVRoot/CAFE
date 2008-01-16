#ifndef _EVENTTYPE_H
#define _EVENTTYPE_H

#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <cctype>		// for size_t

#include "Variable.h"

class EventType
{
	public:
		EventType();
		EventType(const EventType &eventCopy);
		EventType(const string &eventTypeName,
			  const map<string, Variable> &eventVars);

		void GetConfigInfo(string &FileLine, fstream &ReadData);

                // right now, they do the same thing, but there should be a difference.
                // ValidConfig() will return whether it was able to load a configuration correctly.
                // IsValid() will return whether the data contained is valid information.
                bool ValidConfig() const;
                bool IsValid() const;

		const map<string, Variable>& GiveEventVariables() const;
		const string& GiveEventTypeName() const;

		vector <string> GiveEventVariableNames() const;

		size_t GiveLevelCount(const string &EventVarName) const;
		vector <string> GiveEventLevels(const string &EventVarName) const;

		size_t GiveVariableCount() const;

		bool AddVariable(const Variable &NewVariable);
		Variable RemoveVariable(const string &EventVarName);
		
	private:
		string myEventTypeName;
		map<string, Variable> myVariables;

		bool myIsConfigured;

		vector<string> InitTagWords() const;

	friend bool operator == (const EventType &Lefty, const EventType &Righty);
	friend bool operator != (const EventType &Lefty, const EventType &Righty);

	friend bool operator < (const EventType &TheEvent, const string &EventTypeName);
	friend bool operator > (const EventType &TheEvent, const string &EventTypeName);
	friend bool operator <= (const EventType &TheEvent, const string &EventTypeName);
	friend bool operator >= (const EventType &TheEvent, const string &EventTypeName);
	friend bool operator == (const EventType &TheEvent, const string &EventTypeName);
	friend bool operator != (const EventType &TheEvent, const string &EventTypeName);

	friend bool operator < (const string &EventTypeName, const EventType &TheEvent);
	friend bool operator > (const string &EventTypeName, const EventType &TheEvent);
	friend bool operator <= (const string &EventTypeName, const EventType &TheEvent);
	friend bool operator >= (const string &EventTypeName, const EventType &TheEvent);
	friend bool operator == (const string &EventTypeName, const EventType &TheEvent);
	friend bool operator != (const string &EventTypeName, const EventType &TheEvent);
};

class EventTypeID_t
{
	public:
		EventTypeID_t();
		EventTypeID_t(const size_t &EventTypeIndex);
		EventTypeID_t(const string &EventTypeName);

		size_t GiveIndex(const vector <EventType> &EventTypes) const;
		string GiveName() const;

	private:
		size_t myEventTypeIndex;
		string myEventTypeName;
};


#endif
