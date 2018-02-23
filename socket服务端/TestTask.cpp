
#include "TestTask.h"
#include "mcom.h"
#include "stdio.h"
#include "SaveQueue.h"
#include<ctime> 

#ifdef  WIN_64_DEF
#include <mysql.h>
#endif

#ifdef  WIN_64_DEF
#pragma comment(lib,"libmysql.lib") 
#endif
//////////////////////////////////////////////////////////////////////
CMySaveQueue   m_SaveQueueSQL;//创建对象，实例化一个结构体队列

CTestTask *pTask =new CTestTask(10);//创建任务
int  saveCNT=0;
//////////////////////////////////////////////////////////////////////
CTestTask::CTestTask(int id)
	:CTask(id)
{

}
CTestTask::~CTestTask(void)
{
}
////////////////////硬件接收数据解包///////////////////////////////
void CTestTask::taskRecClientProc()
{
	RecClientData(m_SocReUnit.SocketNum ,  m_SocReUnit.RecData ,m_SocReUnit.DataLen);
}

////////////////////接收APP数据解包///////////////////////////////
void CTestTask::taskAPPRecProc()
{
	RecAPPData(mAPP_RevUnit.SocketNum ,  mAPP_RevUnit.RecData ,mAPP_RevUnit.DataLen);
}
///////////////////硬件解析后数据的存储///////////////////////////////
void CTestTask::taskSaveSQL()
{
	HardData  mdata , *pmdata;

	EnterCriticalSection(&mSaveDataLock);//待存储数据加锁

	if(!m_SaveQueueSQL.isEmpty())//判断队列非空
	{
		HardData  *pd = m_SaveQueueSQL.pop();//解析后的队列数据出队一组
		if(pd==NULL)
		{
			LeaveCriticalSection(&mSaveDataLock);//解锁
			std::cout<<"数据队列出队失败!!!"<<std::endl;
			
		}else
		{
			memcpy(mdata.RecData , pd->RecData ,  pd->DataLen);
			mdata.DataLen =  pd->DataLen;	
			mdata.cmd =  pd->cmd;
			mdata.SocketNum =  pd->SocketNum;
			pmdata = &mdata;

			LeaveCriticalSection(&mSaveDataLock);//解锁

			SaveDataSQL( pmdata->SocketNum , pmdata->cmd,  (char*)pmdata->RecData , pmdata->DataLen);//采集数据包中的实际用户数据信息
		
		}
	}
	else
	{
		LeaveCriticalSection(&mSaveDataLock);//解锁
	}
	

}

/************************数据比较*****************************************/
unsigned  char    DataS_Compare(unsigned  char * p1  ,unsigned  char  *p2, unsigned  short  len)
{
 	unsigned 	short i=0;
	for(i=0;i<len;i++)
	{
		if(*(p1++) == *(p2++) )
		{

		}
		else
		return 0 ; //比较不同为0
	 	
	}

	return  1 ;
}

/**************************查找制定连续个字节*************************************/
unsigned  char  * FindByteBuff(unsigned  char  *src , unsigned  long srclen ,  unsigned char *dec , unsigned  short datalen )
{
	unsigned  char *pt = dec;
	unsigned	char *ps = src;
	unsigned	char *pd = dec;
	int c = 0;

	while( (unsigned  long )(ps-src) < srclen )
	{
			if(*ps == *pd) //查找到首个相同
			{
					
					while(  (datalen >c) && (srclen-(ps-src)) >0 )	//接着查找剩余的个数
					{
							if(*(ps++) == *(pd++) )
							{
								c++;
							}
							else //不相同，半途而废
							{
								c=0;
								break;
							}
					}
			}

			else	//不相同继续下移查找
			{
					ps++;
			}

			if( c == datalen)	 //如果查找到结尾，达到了所有相同的个数
			{						
				return (unsigned  char *)(ps - c);  //返回首字符位置
			}
			
			if( (ps -src)== srclen )
			return 0;  //返回失败
			
			c = 0;	 //
			pd = pt; //
			
	}

	return 0;
	
}

/////////////////////////////解析出来的硬件数据压入队列，并添加存储数据任务队列////////////////////////////////////////////////
char  SaveClientData(SOCKET   ClientS ,int cmd , unsigned  char * src, unsigned  int  len)
{
	HardData  mdata , *pmdata ;

	EnterCriticalSection(&mSaveDataLock);//待存储数据队列操作，公共对象先加锁
	
	memcpy(mdata.RecData , src , len);
	mdata.DataLen = len;	
	mdata.cmd = cmd;
	mdata.SocketNum = ClientS;
	pmdata= &mdata;
	
	//具体数据压入待存储队列
	m_SaveQueueSQL.push(pmdata);//待存储的数据入队，全局共用变量要加锁操作，因为该函数在多线程中调用了
	//这里的任务队列其实就是存了需要执行的任务次数，触发一次，就入队一次。实际数据没有存储，是全局变量						
	SaveDatathreadpool.addTask(pTask,NORMAL);//任务放入线程池中并执行任务，会唤醒挂起的线程，从而执行任务类的具体代码
	//saveCNT++;
	//printf("数据入队成功 %d\n",saveCNT);//打印卡号
	LeaveCriticalSection(&mSaveDataLock);//解锁

	return 1;
	
}

/***********************************************************************
char   RecClientData
接收解析硬件上传数据
************************************************************************/
char   RecClientData(SOCKET ClientS, unsigned char * src, unsigned  int  len )
{
	unsigned  char decdata[6]={ 0x96,0x69,0x96,0x69};
	char  loop=1;
	unsigned  char *pstr;
	unsigned  char *srcdat=src;
	unsigned  char *pdata = src;
	long  DataLen=0;
	long  length=0;

	SocketRecUnit   mRECdataS; //数据缓存结构体，包括socket连接

	EnterCriticalSection(&m_RECdataLock);//硬件解包线程加锁，保护全局变量
	memcpy( mRECdataS.RecData , src  ,len );
	mRECdataS.DataLen = len;
	
	srcdat=mRECdataS.RecData;
	pdata = srcdat;
	DataLen = mRECdataS.DataLen;
	LeaveCriticalSection(&m_RECdataLock);//解锁

	if(DataLen >0 && DataLen < REC_SIZE)
	{	
		while(loop)
		{
			if( (DataLen-(pdata-srcdat)) >= APP_MIN_DATA_LEN )
			{
				pstr = FindByteBuff(pdata , (DataLen-(pdata-srcdat)) , decdata , 4);//查找命令头
				
				if(pstr!=0)//查找到包头
				{		
					pdata=pstr;
					length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
					if( length >= 0 )//判断剩余数据长度是否够一个整包
					{
						switch (*(pdata+4))//判断命令头
						{								
							case  CLIENT_TO_SEVER :													
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;					
							break;
							case   CLIENT_GPS_INFO:  //GPS上传
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//判断剩余数据长度
								else
								loop=0;	
							break;
							case   CLIENT_BASE_STATION_INFO:  //基站信息上传
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//解析网络数据
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//判断剩余数据长度
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//判断剩余数据长度
								else
								loop=0;	
							break;
							default: 
							loop =0;	
							break;
						}
					}
					else
					{
						loop=0;
					}
				}
				else
				{
						loop=0;
				}
			}
			else
			{
				loop=0;
			}
		}


			return  1;
	}
	else
	{
		
			return 0;
	}
	
}
//////////////////////////////////具体的硬件数据存储数据库函数////////////////////////////////////////////
char  SaveDataSQL(SOCKET   ClientS ,int Command, char * src, unsigned  int  len)
{
	
	unsigned  int  i=0;
	string str_card ;
	int  ret=0;

	Json::Value  m_json;
	Json::Reader *pJsonParser;
	
	switch(Command)
	{
		case   CLIENT_GPS_INFO: //硬件上传GPS
			#ifdef  WIN_64_DEF
			//SaveHardStateInfo(ClientS , m_json );
			
			SaveGPSData(ClientS , (unsigned  char *)src , len);
			#endif
			return 0;
		break;

		case   CLIENT_TO_SEVER :	//
		/***************由于Jsoncpp解析非法json时，会自动容错成字符类型。对字符类型取下标时，会触发assert终止进程。
		解决方法：启用严格模式，让非法的json解析时直接返回false，不自动容错。这样，在调用parse的时候就会返回false。
		*************************/
			#ifdef  WIN_64_DEF
			pJsonParser = new Json::Reader(Json::Features::strictMode());//实例化

			if(!pJsonParser->parse(src, m_json)) //char* 转json并做合法性检查
			{
				cout << "parse error" << endl;
				delete  pJsonParser;
				return -5;
			}
			delete  pJsonParser;
			//ParseJsonFromString(src, &m_json);//char* 转json
			//cout<<m_json<<endl;
			//Json::FastWriter  fast_writer;//查看json内容对象
			//string str = fast_writer.write( m_json); //json转string
			//cout<<str<<endl;//这里是带引号的""，当做string内容
			//ret = FindToken(m_json["token"].asString());//比对token
			//static  int  reccnt=0;//只会初始化一次
			//reccnt++;
			//cout<<"验证结束啦啦:  "<<totalcnt<<endl;
			
			SaveHardStateInfo(ClientS , m_json );
			#endif
			return 0;
		break;
		case  CLIENT_BASE_STATION_INFO: //硬件上传无GPS的基站信息

			SaveBaseStationData(ClientS , (unsigned  char *)src , len);

			break;
		default: 
			cout<<"队列数据命令字错误！"<<endl;				
			return -4;
		break;
	
	}

	return 1;
	//printf("数据存储\n");
	//for(int i=0;i<4;i++)
	//printf("%02x", *(src+i));

	//printf("\n");

}
//////////////////////////////////////////////////////////////////////////////
int  SaveHardStateInfo(SOCKET   ClientS , Json::Value  mJsonValue)
{
	//time_t now_time; 
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   
	
	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	my_ulonglong  num_row,num_col; 

	string   str_soc,str_username ,str_card,str_state,str_lock,str_gps,str_time; 

	if( GetVaule_CheckStr(&str_card , mJsonValue , "card") == 0 )
	{
		cout<< "json数据类型错误card应该为string"<<endl;
		return  -5;
	}
	
	str_soc = to_string((_ULonglong)ClientS);//socket连接号,整型转string
	
	if( GetVaule_CheckStr(&str_state , mJsonValue , "card_state") == 0 )
	{
		cout<< "json数据类型错误card_state应该为string"<<endl;
		return  -5;
	}

	if(mJsonValue["card_lock"].isInt())
		str_lock = mJsonValue["card_lock"].asInt() + '0';
	else
	{		
		cout<< "json数据value类型错误card_lock应该为int"<<endl;
		return  -5;

	}

	if( GetVaule_CheckStr(&str_gps , mJsonValue , "gps") == 0 )
	{
		cout<< "json数据类型错误gps应该为string"<<endl;
		return  -5;
	}

	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";
	
	//now_time = time(NULL); 
	//cout<<now_time<<endl; 

	//SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{			
		
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >0 )
		{
			getsucess =true;
	
		}
		else
			getsucess = false;

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		cout << "select username error!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  card_data(  card , card_socket, card_state , card_lock , gps ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_gps+ "', NOW() ) ";

	//UPDATE不会新增,是覆盖,有几条就覆盖几条。
	
	//string  mSQLStr="UPDATE  user_bike  SET  username = '" + str_username + "', card = '" + str_card + "', phone = '" + str_phone +"', bikename = '" + str_bikename +
	//	"', bikecolor = '" + str_bikecolor + "', biketype = '"+str_biketype + "', regester_time ='2017-5-10-10:02:05' ";

	//cout<<mSQLStr<<endl;

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	mysql_close(&myCont);//及时关闭mysql连接，否则占用连接数
	if(!res  )
	{	
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string
			
		send(ClientS , (char *)str.data(), (int)str.length() , 0);  // 发送信息 
		//_sleep(500);
		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		//cout << "///////sucess!\n" << endl;
		return 0;  		
	}
	else
	{
	
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("update_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(),(int)str.length(), 0);  // 发送信息 
		cout << "add  SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	
	return -1;

}


////////////////////////////保存GPS数据////////////////////////////////////////
int   SaveGPSData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{
	//time_t now_time; 
	int i=0;
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	int  num_row,num_col; 

	string   str_soc,str_username ,str_card,str_state,str_lock,str_gps,str_time; 

	char  temp[200];
	for(i=0;i<4;i++)
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //小写16 进制，宽度占2个位置，右对齐，不足补0
	
	temp[8]='\0';
	//str_card = getNullStr(temp);
	str_card = temp;
	//if( *(src+2+3)< 50 )
	//printf("%s\n",str_card);//打印卡号

	//EnterCriticalSection(&card_list_Lock);//待存储数据加锁

	//card_list  +=str_card;
	//card_list += "\r\n";
	//LeaveCriticalSection(&card_list_Lock);//解锁
	//send(ClientS , (char *)(src+2), 8, 0);  // 发送信息 
	//return 0;
	
	str_soc = to_string((_Longlong)ClientS);//socket连接号,整型转string
	
	if('O'==*(src+6))
	str_state="Out";
	else
	str_state="In";

	if(1 ==*(src+7))
	str_lock= "1";
	else
	str_lock= "0";
	if(len-8 > 200)
		len=200;
	memcpy(temp,src+8,len-8);
	temp[len-8]='\0';
	str_gps = getNullStr(temp);

	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{				
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		cout << "GPS---查询card失败!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  card_data(  card , card_socket, card_state , card_lock , gps ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_gps+ "', NOW() ) ";

	//UPDATE不会新增,是覆盖,有几条就覆盖几条。

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	
	mysql_close(&myCont);//及时关闭mysql连接，否则占用连接数
	if(!res  )
	{		
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 	

		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
		cout << "SaveGPS-----sucess!\n" << endl;
		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("update_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "add GPS SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	
	return -1;

}

////////////////////////////保存基站数据////////////////////////////////////////
int   SaveBaseStationData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{
	//time_t now_time; 
	int i=0;
	bool  getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";    
    char table[] = "bike";  
	unsigned int port = 3306;   

	MYSQL myCont;
    MYSQL_RES *result;
    int res;
	int  num_row,num_col; 

	string   str_soc,str_username ,str_card,str_state,str_lock,str_base_station,str_time; 

	char  temp[200];
	for(i=0;i<4;i++)
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //小写16 进制，宽度占2个位置，右对齐，不足补0
	
	temp[8]='\0';
	//str_card = getNullStr(temp);
	str_card = temp;
	//if( *(src+2+3)< 50 )
	//printf("%s\n",str_card);//打印卡号

	//EnterCriticalSection(&card_list_Lock);//待存储数据加锁

	//card_list  +=str_card;
	//card_list += "\r\n";
	//LeaveCriticalSection(&card_list_Lock);//解锁
	//send(ClientS , (char *)(src+2), 8, 0);  // 发送信息 
	//return 0;
	
	str_soc = to_string((_Longlong)ClientS);//socket连接号,整型转string
	
	if('O'==*(src+6))
	str_state="Out";
	else
	str_state="In";

	if(1 ==*(src+7))
	str_lock= "1";
	else
	str_lock= "0";
	if(len-8 > 200)
		len=200;
	memcpy(temp,src+8,len-8);
	temp[len-8]='\0';
	str_base_station = getNullStr(temp);

	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

    mysql_init(&myCont);//初始化mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //设置编码格式       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //执行SQL语句,通过token查找username
	if(!res  )
	{				
		//保存查询到的数据到result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //读取行数
		num_col=mysql_num_fields(result); //读取列数
		//printf("row: %d,col: %d\n",num_row,num_col);//打印行列个数
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //返回所有字段结构的数组
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //释放缓存，特别注意，不释放会导致内存增长

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		cout << "基站定位---查询card失败!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT 会新增一条
	//on duplicate key update  需要设置一个主键，不自动增长，遇到主键冲突，执行后面的updata内容
	string  mSQLStr = "INSERT  INTO  card_base_station_data(  card , card_socket, card_state , card_lock , base_station ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_base_station+ "', NOW() ) ";

	//UPDATE不会新增,是覆盖,有几条就覆盖几条。

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //执行SQL语句,添加一条记录
	
	mysql_close(&myCont);//及时关闭mysql连接，否则占用连接数
	if(!res  )
	{		
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(0);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("sucess"); // 新建一个 Key（名为：key_number），赋予数值：12345。	
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 	

		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d 星期%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
		cout << "SaveBaseStation-----sucess!\n" << endl;
		return 0;  		
	}
	else
	{
		Json::Value root;             // 表示整个 json 对象
		root["errno"] = Json::Value(1);     // 新建一个 Key（名为：key_string），赋予字符串值："value_string"。
		root["error"] = Json::Value("update_error");             // 新建一个 Key（名为：key_number），赋予数值：12345。
		
		Json::FastWriter  fast_writer;//查看json内容对象
		string str = fast_writer.write(root); //json转string

		//send(ClientS , (char *)str.data(), (int)str.length(), 0);  // 发送信息 
		cout << "Insert 基站数据 SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	
	return -1;

}






