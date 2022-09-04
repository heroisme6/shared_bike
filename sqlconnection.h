#ifndef  DATASTORE_MYSQL_CONNECTION_H
#define DATASTORE_MYSQL_CONNECTION_H 
#include <mysql/mysql.h>
#include <string>
#include <mysql/errmsg.h>
#include <assert.h>
#include "glo_def.h"
class SqlRecordSet {//结果集
public:
	SqlRecordSet() :m_pRes(NULL) {

	}

	explicit SqlRecordSet(MYSQL_RES* pRes) {
		m_pRes = pRes;
	}

	MYSQL_RES* MysqlRes() {
		return m_pRes;
	}

	~SqlRecordSet() {
		if (m_pRes) {
			mysql_free_result(m_pRes);
		}
	}

	inline void SetResult(MYSQL_RES* pRes) {
		assert(m_pRes == NULL);//如果此时已经保存了结果集，那么应该让程序报错，防止内存泄漏(不允许第二次设置)
		if (m_pRes) {
			LOG_WARN("the MTSQL_RES has already stored result, maaybe will cause memory leak");
		}
		m_pRes = pRes;
	}

	inline MYSQL_RES* GetResult() {
		return m_pRes;
	}

	void FetchRow(MYSQL_ROW& row) {
		row = mysql_fetch_row(m_pRes);
	}

	inline i32 GetRowCount() {
		return m_pRes->row_count;
	}


private:
	MYSQL_RES* m_pRes;
};
class MysqlConnection {
public:
	MysqlConnection();
	~MysqlConnection();

	MYSQL* Mysql() {
		return mysql_;
	}

	bool Init(const char* szHost, int nPort, const char* szUser, const char* szPasswd, const char* szDb);//连接

	bool Execute(const char* szSql);//执行增删改查

	bool Execute(const char* szSql, SqlRecordSet& recordSet);//MYSQL_RE*，执行

	int EscapString(const char* pSrc, int nSrcLen, char* pDest); 

	void Close();

	const char* GetErrInfo();

	void Reconnect();//如果断开连接，重新连接

private:
	MYSQL* mysql_;

};

#endif // ! DATASTORE_MYSQL_CONNECTION_H

