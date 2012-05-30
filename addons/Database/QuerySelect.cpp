#include "Database.h"
#include "QuerySelect.h"

namespace roxlu {

QuerySelect::QuerySelect(Database& db)	
	:Query(db)
	,select_fields("*")
	,where_clause("")
    ,group_clause("")
	,order_clause("")
	,limit_clause("")
{
}

QuerySelect::QuerySelect(Database& db, const string& selectFields) 
	:Query(db)
	,select_fields(selectFields)
	,where_clause("")
    ,group_clause("")
	,order_clause("")
	,limit_clause("")
{
	
}

QuerySelect::QuerySelect(const QuerySelect& other) 
	:Query(other.db)
{
	*this = other;
}

QuerySelect& QuerySelect::operator=(const QuerySelect& other) {
	if(this == &other) {
		return *this;
	}
	printf("Copying QuerySelect\n");
	select_fields = other.select_fields;
	from_table = other.from_table;
	join_clause = other.join_clause;
	where_clause = other.where_clause;
    group_clause = other.group_clause;
	order_clause = other.order_clause;
	limit_clause = other.limit_clause;
	field_values = other.field_values;
	return *this;
}

QuerySelect::~QuerySelect() {
}

QuerySelect& QuerySelect::from(const string& table) {
	from_table = table;
	return *this;
}

QuerySelect& QuerySelect::join(const string& joinClause) {
	join_clause = joinClause;
	return *this;
}

QuerySelect& QuerySelect::where(const string& whereClause) {
	where_clause = whereClause;
	return *this;
}

QuerySelect& QuerySelect::order(const string& orderClause) {
	order_clause = orderClause;
	return *this;
}
    
QuerySelect& QuerySelect::group(const string& groupClause) {
    group_clause = groupClause;
    return *this;
}

QuerySelect& QuerySelect::limit(const string& limitClause) {
	limit_clause = limitClause;
	return *this;
}


string QuerySelect::toString() {
	string sql = "";
	sql.append("select ");
	sql.append(select_fields);
	sql.append(" from ");
	sql.append(from_table);
	
	if(join_clause.length()) {
		sql.append(" inner join ");
		sql.append(join_clause);
		sql.append(" ");
	}
	
	if(where_clause.length()) {
		sql.append(" where ");
		sql.append(where_clause);
		sql.append(" ");
	}
    
    if(group_clause.length()) {
		sql.append(" group by ");
		sql.append(group_clause);
	}
		
	if(order_clause.length()) {
		sql.append(" order by ");
		sql.append(order_clause);
	}
	
	if(limit_clause.length()) {
		sql.append(" limit ");
		sql.append(limit_clause);
	}
	
	//printf("SQL: %s\n" , sql.c_str());
	return sql;
	
}

bool QuerySelect::execute(QueryResult& result) {
	string sql = toString();
	if(!sql.length()) {
		return false;
	}
	return result.execute(sql, field_values, Database::QUERY_SELECT);
}

} // roxlu