#ifndef ROXLU_DATABASE_QUERY_SELECTH
#define ROXLU_DATABASE_QUERY_SELECTH

#include "Query.h"
#include <sstream>
#include <string>
#include "QueryResult.h"

using std::string;

namespace roxlu {

class Database;

class QuerySelect : public Query {
public:
	QuerySelect(Database& db);
	QuerySelect(Database& db, const string& selectFields);
	QuerySelect(const QuerySelect& other);
	QuerySelect& operator=(const QuerySelect& other);
	~QuerySelect();
	
	QuerySelect& from(const string& fromTable);
	bool execute(QueryResult& result);
	string toString();
	
	template<typename T>
	QuerySelect& use(const string& fieldName, const T& fieldValue) {
		field_values.use(fieldName, fieldValue);
		return *this;
	}
	
	QuerySelect& where(const string& whereClause);
	QuerySelect& join(const string& joinClause);
    QuerySelect& group(const string& groupClause);
	QuerySelect& order(const string& orderClause);


	// Use: db.select("tag_id").from("tags").where("tag_name in (%s) ",tweet.tags).execute(tag_result);
	template<typename T>
	QuerySelect& where(const string& whereClause, const T& values) {
		// create the value string
		typename T::const_iterator it = values.begin();
		string in_value;
		int count = 0;
		while(it != values.end()) {
			stringstream ss;
			ss << (*it);
			in_value.append("\"");
			in_value.append(ss.str());
			in_value.append("\"");
			++it;
			if(it != values.end()) {
				in_value.append(",");
			}
		}
		// copy the creating string into the where clause
		int result_size = in_value.length() + whereClause.length() +2;
		char* buffer = new char[result_size];
		sprintf(buffer, whereClause.c_str(), in_value.c_str() );
		where_clause.assign(buffer, result_size);
		delete[] buffer;
		
		return *this;
	}
	
	template<typename T>
	QuerySelect& limit(const T& t) {
		std::stringstream ss;
		ss << t;
		limit(ss.str());
		return *this;
	}
	
	QuerySelect& limit(const string& limitClause);
	
private:
	string select_fields;
	string from_table;
	string join_clause;
	string where_clause;
    string group_clause;
	string order_clause;
	string limit_clause;
	
	QueryParams field_values;
};

} // roxlu
#endif
