
#include <winsock2.h>
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
CMySaveQueue   m_SaveQueueSQL;//��������ʵ����һ���ṹ�����

CTestTask *pTask =new CTestTask(10);//��������
int  saveCNT=0;
//////////////////////////////////////////////////////////////////////
CTestTask::CTestTask(int id)
	:CTask(id)
{

}
CTestTask::~CTestTask(void)
{
}
////////////////////////��ʱ��ѯ�����������߳�////////////////////////////////////
DWORD  WINAPI  CheckHardConfigThread(LPVOID lpParameter)
{
	int ret1, ret2, ret3;
	int CheckCnt = 3;
	int CheckRet1 = 10, CheckRet2 = 10, CheckRet3 = 10;
	int nIndex = 0;
	while (true)
	{
		//ÿ�ε�5000����   
		nIndex = WaitForMultipleObjects(1, CheckHardConfigEvent, FALSE, 2000);

		if (nIndex == WAIT_OBJECT_0) //��һ���¼�����    
		{
			printf("�߳���������ѯ�·�ָ���\r\n");
			CheckCnt = 1;
			CheckRet1 = 3;
			CheckRet2 = 3;
			CheckRet3 = 3;
			while (CheckCnt--)
			{
				if (CheckRet1 > 0)
				{
					ret1 = WX_SendBufang_ToHard();
					switch (ret1)
					{
					case 0://ִ�����
						CheckCnt = 3;
						break;
					case -1://���ñ��пգ�û����Ҫ�·�������
						CheckRet1 = 0;
						break;
					}
				}
				if (CheckRet2 > 0)
				{
					ret2 = WX_Send_MotorLock();
					switch (ret2)
					{
					case 0://ִ�����
						CheckCnt = 3;
						break;
					case -1://���ñ��пգ�û����Ҫ�·�������
						CheckRet2 = 0;
						break;
					}
				}
				if (CheckRet3 > 0)
				{
					ret3 = WX_Send_DeviceOpenToHard();
					switch (ret3)
					{
					case 0://ִ�����
						CheckCnt = 3;
						break;
					case -1://���ñ��пգ�û����Ҫ�·�������
						CheckRet3 = 0;
						break;
					}
				}
				if (!CheckRet1 && !CheckRet2 && !CheckRet3)
					break;
			}
			
		}
		else if (nIndex == WAIT_TIMEOUT) //��ʱ    
		{   //��ʱ������ʱ��    
			Sleep(50);
			//SetEvent(CheckHardConfigEvent[0]);//�����¼������߳�
		}
	}
	printf("�߳̽���\n");

	return 0;

}
////////////////////Ӳ���������ݽ��///////////////////////////////
void CTestTask::taskRecClientProc()
{
	RecClientData(m_SocReUnit.SocketNum ,  m_SocReUnit.RecData ,m_SocReUnit.DataLen);
}

////////////////////����APP���ݽ��///////////////////////////////
void CTestTask::taskAPPRecProc()
{
	RecAPPData(mAPP_RevUnit.SocketNum ,  mAPP_RevUnit.RecData ,mAPP_RevUnit.DataLen);
}
///////////////////Ӳ�����������ݵĴ洢///////////////////////////////
void CTestTask::taskSaveSQL()
{
	//HardData  mdata;
	HardData  *pmdata;

	EnterCriticalSection(&mSaveDataLock);//���洢���ݼ���

	if(!m_SaveQueueSQL.isEmpty())//�ж϶��зǿ�
	{
		HardData  *pd = m_SaveQueueSQL.pop();//������Ķ������ݳ���һ��
		if(pd==NULL)
		{
			LeaveCriticalSection(&mSaveDataLock);//����
			std::cout<<"���ݶ��г���ʧ��!!!"<<std::endl;
			
		}else
		{
			//memcpy(mdata.RecData , pd->RecData ,  pd->DataLen);
			//mdata.DataLen =  pd->DataLen;	
			//mdata.cmd =  pd->cmd;
			//mdata.SocketNum =  pd->SocketNum;
			//pmdata = &mdata;
			pmdata = pd;

			LeaveCriticalSection(&mSaveDataLock);//����

			SaveDataSQL( pmdata->SocketNum , pmdata->cmd,  (char*)pmdata->RecData , pmdata->DataLen);//�ɼ����ݰ��е�ʵ���û�������Ϣ
		
		}
	}
	else
	{
		LeaveCriticalSection(&mSaveDataLock);//����
	}

}

/************************���ݱȽ�*****************************************/
unsigned  char    DataS_Compare(unsigned  char * p1  ,unsigned  char  *p2, unsigned  short  len)
{
 	unsigned 	short i=0;
	for(i=0;i<len;i++)
	{
		if(*(p1++) == *(p2++) )
		{

		}
		else
		return 0 ; //�Ƚϲ�ͬΪ0
	 	
	}

	return  1 ;
}

/**************************�����ƶ��������ֽ�*************************************/
unsigned  char  * FindByteBuff(unsigned  char  *src , unsigned  long srclen ,  unsigned char *dec , unsigned  short datalen )
{
	unsigned  char *pt = dec;
	unsigned	char *ps = src;
	unsigned	char *pd = dec;
	int c = 0;

	while( (unsigned  long )(ps-src) < srclen )
	{
			if(*ps == *pd) //���ҵ��׸���ͬ
			{
					
					while(  (datalen >c) && (srclen-(ps-src)) >0 )	//���Ų���ʣ��ĸ���
					{
							if(*(ps++) == *(pd++) )
							{
								c++;
							}
							else //����ͬ����;����
							{
								c=0;
								break;
							}
					}
			}

			else	//����ͬ�������Ʋ���
			{
					ps++;
			}

			if( c == datalen)	 //������ҵ���β���ﵽ��������ͬ�ĸ���
			{						
				return (unsigned  char *)(ps - c);  //�������ַ�λ��
			}
			
			if( (ps -src)== srclen )
			return 0;  //����ʧ��
			
			c = 0;	 //
			pd = pt; //
			
	}

	return 0;
	
}

/////////////////////////////����������Ӳ������ѹ����У�����Ӵ洢�����������////////////////////////////////////////////////
char  SaveClientData(SOCKET   ClientS ,int cmd , unsigned  char * src, unsigned  int  len)
{
	HardData  mdata , *pmdata ;

	EnterCriticalSection(&mSaveDataLock);//���洢���ݶ��в��������������ȼ���
	
	memcpy(mdata.RecData , src , len);
	mdata.DataLen = len;	
	mdata.cmd = cmd;
	mdata.SocketNum = ClientS;
	pmdata= &mdata;
	
	//��������ѹ����洢����
	m_SaveQueueSQL.push(pmdata);//���洢��������ӣ�ȫ�ֹ��ñ���Ҫ������������Ϊ�ú����ڶ��߳��е�����
	//��������������ʵ���Ǵ�����Ҫִ�е��������������һ�Σ������һ�Ρ�ʵ������û�д洢����ȫ�ֱ���						
	SaveDatathreadpool.addTask(pTask,NORMAL);//��������̳߳��в�ִ�����񣬻ỽ�ѹ�����̣߳��Ӷ�ִ��������ľ������
	//saveCNT++;
	//printf("������ӳɹ� %d\n",saveCNT);//��ӡ����
	LeaveCriticalSection(&mSaveDataLock);//����

	return 1;
	
}

/***********************************************************************
char   RecClientData
���ս���Ӳ���ϴ�����
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

	SocketRecUnit   mRECdataS; //���ݻ���ṹ�壬����socket����

	EnterCriticalSection(&m_RECdataLock);//Ӳ������̼߳���������ȫ�ֱ���
	memcpy( mRECdataS.RecData , src  ,len );
	mRECdataS.DataLen = len;
	
	srcdat=mRECdataS.RecData;
	pdata = srcdat;
	DataLen = mRECdataS.DataLen;
	LeaveCriticalSection(&m_RECdataLock);//����

	if(DataLen >0 && DataLen < REC_SIZE)
	{	
		while(loop)
		{
			if( (DataLen-(pdata-srcdat)) >= APP_MIN_DATA_LEN )
			{
				pstr = FindByteBuff(pdata , (DataLen-(pdata-srcdat)) , decdata , 4);//��������ͷ
				
				if(pstr!=0)//���ҵ���ͷ
				{		
					pdata=pstr;
					length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
					if( length >= 0 )//�ж�ʣ�����ݳ����Ƿ�һ������
					{
						switch (*(pdata+4))//�ж�����ͷ
						{								
							case  CLIENT_TO_SEVER :													
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//������������
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//�ж�ʣ�����ݳ���
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;
								else
								loop=0;					
							break;
							case   CLIENT_GPS_INFO:  //GPS�ϴ�
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//������������
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//�ж�ʣ�����ݳ���
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//�ж�ʣ�����ݳ���
								else
								loop=0;	
							break;
							case   CLIENT_BASE_STATION_INFO:  //��վ��Ϣ�ϴ�
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//������������
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//�ж�ʣ�����ݳ���
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//�ж�ʣ�����ݳ���
								else
								loop=0;	
							break;
							case  CLIENT_ALARM_INFO:
								SaveClientData( ClientS , *(pdata+4) ,(pdata+DATA_HEAD_LEN) , *(pdata+14)*256+ *(pdata+15) );//������������
								length =  DataLen-(pdata-srcdat)- (DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN);
								if( length >= APP_MIN_DATA_LEN )//�ж�ʣ�����ݳ���
								pdata = pdata+DATA_HEAD_LEN+*(pdata+14)*256+ *(pdata+15)+DATA_END_LEN;//�ж�ʣ�����ݳ���
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
//////////////////////////////////�����Ӳ�����ݴ洢���ݿ⺯��////////////////////////////////////////////
char  SaveDataSQL(SOCKET   ClientS ,int Command, char * src, unsigned  int  len)
{
	
	unsigned  int  i=0;
	string str_card ;
	int  ret=0;

	Json::Value  m_json;
	Json::Reader *pJsonParser;
	
	switch(Command)
	{
		case   CLIENT_GPS_INFO: //Ӳ���ϴ�GPS
			#ifdef  WIN_64_DEF
			//SaveHardStateInfo(ClientS , m_json );
			
			SaveGPSData(ClientS , (unsigned  char *)src , len);
			#endif
			return 0;
		break;

		case   CLIENT_TO_SEVER :	//
		/***************����Jsoncpp�����Ƿ�jsonʱ�����Զ��ݴ���ַ����͡����ַ�����ȡ�±�ʱ���ᴥ��assert��ֹ���̡�
		��������������ϸ�ģʽ���÷Ƿ���json����ʱֱ�ӷ���false�����Զ��ݴ��������ڵ���parse��ʱ��ͻ᷵��false��
		*************************/
			#ifdef  WIN_64_DEF
			pJsonParser = new Json::Reader(Json::Features::strictMode());//ʵ����

			if(!pJsonParser->parse(src, m_json)) //char* תjson�����Ϸ��Լ��
			{
				cout << "parse error" << endl;
				delete  pJsonParser;
				return -5;
			}
			delete  pJsonParser;
			//ParseJsonFromString(src, &m_json);//char* תjson
			//cout<<m_json<<endl;
			//Json::FastWriter  fast_writer;//�鿴json���ݶ���
			//string str = fast_writer.write( m_json); //jsonתstring
			//cout<<str<<endl;//�����Ǵ����ŵ�""������string����
			//ret = FindToken(m_json["token"].asString());//�ȶ�token
			//static  int  reccnt=0;//ֻ���ʼ��һ��
			//reccnt++;
			//cout<<"��֤��������:  "<<totalcnt<<endl;
			
			SaveHardStateInfo(ClientS , m_json );
			#endif
			return 0;
		break;
		case  CLIENT_BASE_STATION_INFO: //Ӳ���ϴ���GPS�Ļ�վ��Ϣ
			SaveBaseStationData(ClientS , (unsigned  char *)src , len);
			break;
		case  CLIENT_ALARM_INFO:  //���汨����Ϣ
			SaveAlarmData(ClientS , (unsigned  char *)src , len);
			break;
		default: 
			cout<<"�������������ִ���"<<endl;				
			return -4;
		break;
	
	}

	return 1;
	//printf("���ݴ洢\n");
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
		cout<< "json�������ʹ���cardӦ��Ϊstring"<<endl;
		return  -5;
	}
	
	str_soc = to_string((long)ClientS);//socket���Ӻ�,����תstring
	
	if( GetVaule_CheckStr(&str_state , mJsonValue , "card_state") == 0 )
	{
		cout<< "json�������ʹ���card_stateӦ��Ϊstring"<<endl;
		return  -5;
	}

	if(mJsonValue["card_lock"].isInt())
		str_lock = mJsonValue["card_lock"].asInt() + '0';
	else
	{		
		cout<< "json����value���ʹ���card_lockӦ��Ϊint"<<endl;
		return  -5;

	}

	if( GetVaule_CheckStr(&str_gps , mJsonValue , "gps") == 0 )
	{
		cout<< "json�������ʹ���gpsӦ��Ϊstring"<<endl;
		return  -5;
	}

	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";
	
	//now_time = time(NULL); 
	//cout<<now_time<<endl; 

	//SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//��ʼ�����ݿ�  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		////cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
    mysql_init(&myCont);//��ʼ��mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
        //mysql_query(&myCont, "SET NAMES utf8"); //���ñ����ʽ       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if(!res  )
	{			
		
		//�����ѯ�������ݵ�result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //��ȡ����
		num_col=mysql_num_fields(result); //��ȡ����
		//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if(num_row >0 )
		{
			getsucess =true;
	
		}
		else
			getsucess = false;

		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		cout << "select username error!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT ������һ��
	//on duplicate key update  ��Ҫ����һ�����������Զ�����������������ͻ��ִ�к����updata����
	string  mSQLStr = "INSERT  INTO  card_data(  card , card_socket, card_state , card_lock , gps ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_gps+ "', NOW(3) ) ";

	//UPDATE��������,�Ǹ���,�м����͸��Ǽ�����
	
	//string  mSQLStr="UPDATE  user_bike  SET  username = '" + str_username + "', card = '" + str_card + "', phone = '" + str_phone +"', bikename = '" + str_bikename +
	//	"', bikecolor = '" + str_bikecolor + "', biketype = '"+str_biketype + "', register_time ='2017-5-10-10:02:05' ";

	//cout<<mSQLStr<<endl;

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //ִ��SQL���,���һ����¼
	mysql_close(&myCont);//��ʱ�ر�mysql���ӣ�����ռ��������
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	if(!res  )
	{	
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring
			
		send(ClientS , (char *)str.data(), (int)str.length() , 0);  // ������Ϣ 
		//_sleep(500);
		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 

		//cout << "///////sucess!\n" << endl;
		return 0;  		
	}
	else
	{
	
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(1);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("update_error");             // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��
		
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring

		send(ClientS , (char *)str.data(),(int)str.length(), 0);  // ������Ϣ 
		cout << "add  SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	return -1;

}


////////////////////////////����GPS����////////////////////////////////////////
int   SaveGPSData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{
	//time_t now_time; 
	int ret1=1 ,ret2=1,ret3 =1 ;
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
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //Сд16 ���ƣ����ռ2��λ�ã��Ҷ��룬���㲹0
	
	temp[8]='\0';
	//str_card = getNullStr(temp);
	str_card = temp;
	//if( *(src+2+3)< 50 )
	//printf("%s\n",str_card);//��ӡ����

	//EnterCriticalSection(&card_list_Lock);//���洢���ݼ���

	//card_list  +=str_card;
	//card_list += "\r\n";
	//LeaveCriticalSection(&card_list_Lock);//����
	//send(ClientS , (char *)(src+2), 8, 0);  // ������Ϣ 
	//return 0;
	
	str_soc = to_string((long)ClientS);//socket���Ӻ�,����תstring
	
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
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//��ʼ�����ݿ�  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
  
	if (mysql_init(&myCont) == NULL)//��ʼ��mysql
	{
		printf("inital mysql handle error");
		return -11;
	}
    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //���ñ����ʽ       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if(!res  )
	{				
		//�����ѯ�������ݵ�result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //��ȡ����
		num_col=mysql_num_fields(result); //��ȡ����
		//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		cout << "GPS---��ѯcardʧ��!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT ������һ��
	//on duplicate key update  ��Ҫ����һ�����������Զ�����������������ͻ��ִ�к����updata����
	string  mSQLStr = "INSERT  INTO  card_data(  card , card_socket, card_state , card_lock , gps ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_gps+ "', NOW(3) ) ";

	//UPDATE��������,�Ǹ���,�м����͸��Ǽ�����

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //ִ��SQL���,���һ����¼
	
	mysql_close(&myCont);//��ʱ�ر�mysql���ӣ�����ռ��������
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	if(!res  )
	{		
		//cout << "SaveGPS-----sucess!\n" << endl;
	    //ret1 = WX_SendBufang_ToHard();
		//ret2 = WX_Send_MotorLock();
		//ret3 = WX_Send_DeviceOpenToHard();
		//if(!ret1 || !ret2 || !ret3)
		//	return 0; 
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // ������Ϣ 	
		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 		
		return 0;  		
	}
	else
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(1);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("insert_gps_error");             // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��
		
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring

		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // ������Ϣ 
		cout << "Insert GPS Data error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	return -1;

}

////////////////////////////�����վ����////////////////////////////////////////
int   SaveBaseStationData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{
	//time_t now_time; 
	int ret1, ret2, ret3;
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
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //Сд16 ���ƣ����ռ2��λ�ã��Ҷ��룬���㲹0
	
	temp[8]='\0';
	//str_card = getNullStr(temp);
	str_card = temp;
	//if( *(src+2+3)< 50 )
	//printf("%s\n",str_card);//��ӡ����

	//EnterCriticalSection(&card_list_Lock);//���洢���ݼ���

	//card_list  +=str_card;
	//card_list += "\r\n";
	//LeaveCriticalSection(&card_list_Lock);//����
	//send(ClientS , (char *)(src+2), 8, 0);  // ������Ϣ 
	//return 0;
	
	str_soc = to_string((long)ClientS);//socket���Ӻ�,����תstring
	
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
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//��ʼ�����ݿ�  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		////cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
    mysql_init(&myCont);//��ʼ��mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //���ñ����ʽ       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if(!res  )
	{				
		//�����ѯ�������ݵ�result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //��ȡ����
		num_col=mysql_num_fields(result); //��ȡ����
		//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		cout << "��վ��λ---��ѯcardʧ��!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT ������һ��
	//on duplicate key update  ��Ҫ����һ�����������Զ�����������������ͻ��ִ�к����updata����
	string  mSQLStr = "INSERT  INTO  card_base_station_data(  card , card_socket, card_state , card_lock , base_station ,time )   VALUES( '" +
		str_card + "'," + str_soc + ",'"+ str_state + "',"+ str_lock + ",'"+ str_base_station+ "', NOW(3) ) ";

	//UPDATE��������,�Ǹ���,�м����͸��Ǽ�����

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //ִ��SQL���,���һ����¼
	
	mysql_close(&myCont);//��ʱ�ر�mysql���ӣ�����ռ��������
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	if(!res  )
	{		
		cout << "SaveBaseStation-----sucess!\n" << endl;
		//ret1 = WX_SendBufang_ToHard();
		//ret2 = WX_Send_MotorLock();
		//ret3 = WX_Send_DeviceOpenToHard();
		//if (!ret1 || !ret2 || !ret3)
		//	return 0;
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // ������Ϣ 	

		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
		cout << "SaveBaseStation-----sucess!\n" << endl;
		return 0;  		
	}
	else
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(1);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("update_error");             // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��
		
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring

		//send(ClientS , (char *)str.data(), (int)str.length(), 0);  // ������Ϣ 
		cout << "Insert ��վ���� SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	return -1;

}
////////////////////////////���汨���б�////////////////////////////////////////
int   SaveAlarmData(SOCKET   ClientS ,  unsigned  char * src ,unsigned  int  len)
{
	//time_t now_time; 
	int ret1, ret2, ret3;
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
		sprintf(&temp[2*i], "%02x", *(src+2+i)); //Сд16 ���ƣ����ռ2��λ�ã��Ҷ��룬���㲹0
	
	temp[8]='\0';
	//str_card = getNullStr(temp);
	str_card = temp;
	
	str_soc = to_string((long)ClientS);//socket���Ӻ�,����תstring
	
	if('1'==*(src+6))
	str_state="Out";
	else
	str_state="In";

	if(1 ==*(src+7))
	str_lock= "1";
	else
	str_lock= "0";
	string str_send ="1";
	string  m_strToken = "SELECT  card  FROM  cardinfo  WHERE card = '" + str_card + "'";

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//��ʼ�����ݿ�  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
    mysql_init(&myCont);//��ʼ��mysql

    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {
		//mysql_query(&myCont, "SET NAMES utf8"); //���ñ����ʽ       
    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		return -2;
    }
///////////////////////////////////////////////////////////////////////////////////
	//cout<<m_strToken<<endl;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if(!res  )
	{				
		//�����ѯ�������ݵ�result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //��ȡ����
		num_col=mysql_num_fields(result); //��ȡ����
		//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if(num_row >0 )
		{
			getsucess =true;
		}
		else
			getsucess = false;

		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����

	}
	if(getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		cout << "������Ϣ����---��ѯcardʧ��!\n" << endl;
		return -4;
	}
	
///////////////////////////////////////////////////////////////////////	
	//INSERT ������һ��
	//on duplicate key update  ��Ҫ����һ�����������Զ�����������������ͻ��ִ�к����updata����
	string  mSQLStr = "INSERT  INTO  card_alarm(  card , card_state , card_lock ,send,time )   VALUES( '" +
		str_card + "', '"+ str_state + "',"+ str_lock + ","+ str_send+ ", NOW(3) ) ";

	res = mysql_query(&myCont, (const  char *)mSQLStr.c_str()); //ִ��SQL���,���һ����¼
	
	mysql_close(&myCont);//��ʱ�ر�mysql���ӣ�����ռ��������
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	if(!res  )
	{		
		cout << "SaveCardAlarm-----sucess!\n" << endl;
		//ret1 = WX_SendBufang_ToHard();
		//ret2 = WX_Send_MotorLock();
		//ret3 = WX_Send_DeviceOpenToHard();
		//if (!ret1 || !ret2 || !ret3)
		//	return 0; 
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring			
		send(ClientS , (char *)str.data(), (int)str.length(), 0);  // ������Ϣ 	

		//GetLocalTime( &sys ); 
		//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
		return 0;  		
	}
	else
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(1);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("save_alarm_error");             // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��
		
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring

		//send(ClientS , (char *)str.data(), (int)str.length(), 0);  // ������Ϣ 
		cout << "Insert Alarm SQL error!\n" << endl;
		return -3;
		
	}
    mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	return -1;

}
//////////////////////���ҵȴ��趨���豸�������͸�Ӳ��//////////////////////////////////////////
int   WX_SendBufang_ToHard( )
{
	SOCKET ClientS;
	bool getsucess =false;
	const char user[] = "root";         
    const char pswd[] = "123456";        
    const char host[] = "localhost";  
	unsigned int port = 3306;   
    char table[] = "bike";    
	//return -1;
	struct tm ;  

	string   tos ,str_token,str_card ,str_gps, str_username;  
	string   str_lock;
	//string   radius;
	//string   weilan_gps;
	//string   weilan_radius;
	string   allow_alarm;
	string  m_strToken ;
	
	            
    MYSQL myCont;
    MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //������
	my_ulonglong  f1,f2,num_row,num_col; 
    int res;

	SYSTEMTIME sys; 
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//��ʼ�����ݿ�  
	if (0 == mysql_library_init(0, NULL, NULL)) 
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else 
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
    mysql_init(&myCont);

	//return -1;
    if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
    {  

    }
    else
    {
        cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		return -2;
    }
	
	//cout << "�����ڴ����� !\n" << endl;
	//return -1;
	//cout<<m_strToken.c_str()<<endl;
	m_strToken = "SELECT  *  FROM  set_card_alarm   ORDER BY time ASC LIMIT 1 ";

	getsucess =false;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if(!res  )
	{			
		
		//�����ѯ�������ݵ�result
        result = mysql_store_result(&myCont);
        num_row=mysql_num_rows(result); //��ȡ����
		num_col=mysql_num_fields(result); //��ȡ����
		//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
	    MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if(num_row > 0)
		{
			for(f1=0;f1<1;f1++) //ѭ����ֻȡ�����µ�һ��
			{		
				mysql_row = mysql_fetch_row(result); //��ȡÿ�е�����

				for(f2=0;f2<num_col;f2++) //ѭ����
				{	
					if (fields[f2].name!=NULL)  
					{						
						if (!strcmp( fields[f2].name , "allow_alarm")) //�жϵ�ǰ�е��ֶ�����
						{
							allow_alarm = getNullStr(mysql_row[f2]); //��ȡ�ֶ����ݣ������жϷ�NULL			
							getsucess =true;
							//cout<<allow_alarm<<endl;
						}
						else if (!strcmp( fields[f2].name , "card")) //�жϵ�ǰ�е��ֶ�����
						{
							str_card = getNullStr(mysql_row[f2]); //��ȡ�ֶ����ݣ������жϷ�NULL			
							getsucess =true;
							//cout<<allow_alarm<<endl;
						}
						
					}					
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //��ӡÿ�еĸ���������			
				}
			}
		}
		
		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����

	}

	if(getsucess ==true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		//cout << "no alarm_set list !\n" << endl;
		return -1;
	}
	
	m_strToken= "SELECT  card_socket  FROM  card_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
	getsucess = false;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if (!res)
	{
		//�����ѯ�������ݵ�result
		result = mysql_store_result(&myCont);
		num_row = mysql_num_rows(result); //��ȡ����
		num_col = mysql_num_fields(result); //��ȡ����
		//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
		MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if (num_row > 0)
		{
			mysql_row = mysql_fetch_row(result); //��ȡÿ�е�����
			if (fields[0].name != NULL)
			{		
				ClientS = atoi(mysql_row[0]); //��ȡ�ֶ����ݣ������жϷ�NULL	
				//cout << "Ӳ��socket�˿ں��ǣ�" << ClientS << endl;
			}

		}
		else
		{
			mysql_close(&myCont);
			mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
			cout << "��������ָ��ʧ�ܣ��豸û���ϴ���¼���ò���socket��" << endl;
			return -22;
		}
		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����
	}
	if( 0 == allow_alarm.compare("1"))
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		root["lock"] = Json::Value("setlock");
		root["card"] = Json::Value(str_card);
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring	
		send(ClientS , str.c_str(), str.length() , 0);  // ������Ϣ
		//cout << "�������ָ�" << endl;
	}		
	else 
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		root["lock"] = Json::Value("setunlock");
		root["card"] = Json::Value(str_card);
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring	
		send(ClientS , str.c_str(), str.length() , 0);  // ������Ϣ
		//cout << "���ͳ���ָ�" << endl;
	}

	m_strToken = "DELETE    FROM  set_card_alarm   WHERE card = '" + str_card + "' ORDER BY time ASC LIMIT 1 ";
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	//cout << m_strToken.c_str() << endl;
	mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	if (!res)
	{
		//cout << "ɾ���ѷ��͵����ָ�" << endl;
		return 0;
	}
	else
		return -2;

	mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	return -123;

}

//////////////////////���ҵȴ��趨���豸�������͸�Ӳ��//////////////////////////////////////////
int   WX_Send_MotorLock( )
{
	SOCKET ClientS;
	bool getsucess = false;
	const char user[] = "root";
	const char pswd[] = "123456";
	const char host[] = "localhost";
	unsigned int port = 3306;
	char table[] = "bike";
	
	struct tm;
	//return -1;
	string   tos, str_token, str_card, str_gps, str_username;
	string   str_lock;
	//string   radius;
	//string   weilan_gps;
	//string   weilan_radius;
	string   allow_alarm;
	string  m_strToken;
	
	MYSQL myCont;
	MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //������
	my_ulonglong  f1, f2, num_row, num_col;
	int res;

	SYSTEMTIME sys;
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//��ʼ�����ݿ�  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
	mysql_init(&myCont);

	if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
	{
	}
	else
	{
		//cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		return -2;
	}
	
	
	m_strToken = "SELECT  *  FROM  set_motor_lock  ORDER BY  time  ASC LIMIT 1 ";

	//cout<<m_strToken.c_str()<<endl;
	getsucess = false;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if (!res)
	{
		//�����ѯ�������ݵ�result
		result = mysql_store_result(&myCont);
		num_row = mysql_num_rows(result); //��ȡ����
		num_col = mysql_num_fields(result); //��ȡ����
											//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
		MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if (num_row > 0)
		{
			for (f1 = 0; f1<1; f1++) //ѭ����ֻȡ�����µ�һ��
			{
				mysql_row = mysql_fetch_row(result); //��ȡÿ�е�����

				for (f2 = 0; f2<num_col; f2++) //ѭ����
				{
					if (fields[f2].name != NULL)
					{
						if (!strcmp(fields[f2].name, "motor_lock")) //�жϵ�ǰ�е��ֶ�����
						{
							allow_alarm = getNullStr(mysql_row[f2]); //��ȡ�ֶ����ݣ������жϷ�NULL			
							getsucess = true;
							//cout <<"motor_lock: "<< allow_alarm << endl;
						}
						else if (!strcmp( fields[f2].name , "card")) //�жϵ�ǰ�е��ֶ�����
						{
							str_card = getNullStr(mysql_row[f2]); //��ȡ�ֶ����ݣ������жϷ�NULL			
							getsucess =true;
							//cout<<allow_alarm<<endl;
						}
					}
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //��ӡÿ�еĸ���������			
				}
			}
		}

		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����

	}

	if (getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		//cout << "no alarm_set list !\n" << endl;
		return -1;
	}
	m_strToken = "SELECT  card_socket  FROM  card_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
	getsucess = false;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if (!res)
	{
		//�����ѯ�������ݵ�result
		result = mysql_store_result(&myCont);
		num_row = mysql_num_rows(result); //��ȡ����
		num_col = mysql_num_fields(result); //��ȡ����
											//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
		MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if (num_row > 0)
		{
			mysql_row = mysql_fetch_row(result); //��ȡÿ�е�����
			if (fields[0].name != NULL)
			{
				ClientS = atoi(mysql_row[0]); //��ȡ�ֶ����ݣ������жϷ�NULL	
				//cout << "Ӳ��socket�˿ں��ǣ�" << ClientS << endl;
			}
			//getsucess = true;		
			
		}
		else
		{
			mysql_close(&myCont);
			mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
			cout << "���͵������ָ��ʧ�ܣ��豸û���ϴ���¼���ò���socket��" << endl;
			return -22;
		}
		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����
	}
	if (0 == allow_alarm.compare("1"))
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		root["motorlock"] = Json::Value("setmotorlock");
		root["card"] = Json::Value(str_card);
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring	
		send(ClientS, str.c_str(), str.length(), 0);  // ������Ϣ
		//cout << "���������ָ�" << endl;
	}
	else
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		root["motorlock"] = Json::Value("setmotorunlock");
		root["card"] = Json::Value(str_card);
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring	
		send(ClientS, str.c_str(), str.length(), 0);  // ������Ϣ
		//cout << "���ͽ������ָ�" << endl;
	}

	{
		m_strToken = "DELETE    FROM  set_motor_lock   WHERE card = '" + str_card + "' ORDER BY time ASC LIMIT 1 ";
		res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		if (!res)
		{
			return 0;

		}
		else
			return -2;
	}

	mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	return -123;

}

//////////////////////���ҵȴ��趨���豸�������͸�Ӳ��//////////////////////////////////////////
int   WX_Send_DeviceOpenToHard()
{
	SOCKET ClientS;
	bool getsucess = false;
	const char user[] = "root";
	const char pswd[] = "123456";
	const char host[] = "localhost";
	unsigned int port = 3306;
	char table[] = "bike";
	
	struct tm;
	//return -1;
	string   tos, str_token, str_card, str_gps, str_username;
	string   str_lock;
	string   allow_alarm;
	string  m_strToken ;
	
	MYSQL myCont;
	MYSQL_RES *result;
	MYSQL_ROW  mysql_row; //������
	my_ulonglong  f1, f2, num_row, num_col;
	int res;

	SYSTEMTIME sys;
	//GetLocalTime( &sys ); 
	//printf( "%4d/%02d/%02d %02d:%02d:%02d.%03d ����%1d\n",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds,sys.wDayOfWeek); 
	//��ʼ�����ݿ�  
	if (0 == mysql_library_init(0, NULL, NULL))
	{
		//cout << "mysql_library_init() succeed" << endl;
	}
	else
	{
		cout << "mysql_library_init() failed" << endl;
		return -1;
	}
	mysql_init(&myCont);

	if (mysql_real_connect(&myCont, host, user, pswd, table, port, NULL, 0))
	{
	}
	else
	{
		cout << "connect failed!\n" << endl;
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		return -2;
	}
	//cout<<m_strToken.c_str()<<endl;
	getsucess = false;
	
	m_strToken = "SELECT  *  FROM  set_device_open  ORDER BY time ASC LIMIT 1 ";

	getsucess = false;
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if (!res)
	{
		//�����ѯ�������ݵ�result
		result = mysql_store_result(&myCont);
		num_row = mysql_num_rows(result); //��ȡ����
		num_col = mysql_num_fields(result); //��ȡ����
											//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
		MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if (num_row > 0)
		{
			for (f1 = 0; f1<1; f1++) //ѭ����ֻȡ�����µ�һ��
			{
				mysql_row = mysql_fetch_row(result); //��ȡÿ�е�����

				for (f2 = 0; f2<num_col; f2++) //ѭ����
				{
					if (fields[f2].name != NULL)
					{
						if (!strcmp(fields[f2].name, "device_open")) //�жϵ�ǰ�е��ֶ�����
						{
							allow_alarm = getNullStr(mysql_row[f2]); //��ȡ�ֶ����ݣ������жϷ�NULL			
							getsucess = true;
							//cout << allow_alarm << endl;
						}
						else if (!strcmp( fields[f2].name , "card")) //�жϵ�ǰ�е��ֶ�����
						{
							str_card = getNullStr(mysql_row[f2]); //��ȡ�ֶ����ݣ������жϷ�NULL			
							getsucess =true;
							//cout<<allow_alarm<<endl;
						}
					}
					//printf("[Row %d,Col %d]==>[%s]\n",f1,f2,mysql_row[f2]); //��ӡÿ�еĸ���������			
				}
			}
		}

		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����

	}

	if (getsucess == true)
	{

	}
	else
	{
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		//cout << "no alarm_set list !\n" << endl;
		return -1;
	}

	m_strToken = "SELECT  card_socket  FROM  card_data  WHERE card = '" + str_card + "'ORDER BY card_id DESC LIMIT 0,1 ";
	res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
	if (!res)
	{
		//�����ѯ�������ݵ�result
		result = mysql_store_result(&myCont);
		num_row = mysql_num_rows(result); //��ȡ����
		num_col = mysql_num_fields(result); //��ȡ����
											//printf("row: %d,col: %d\n",num_row,num_col);//��ӡ���и���
		MYSQL_FIELD* fields = mysql_fetch_fields(result); //���������ֶνṹ������
		if (num_row > 0)
		{
			mysql_row = mysql_fetch_row(result); //��ȡÿ�е�����
			if (fields[0].name != NULL)
			{
				ClientS = atoi(mysql_row[0]); //��ȡ�ֶ����ݣ������жϷ�NULL	
				//cout << "Ӳ��socket�˿ں��ǣ�" << ClientS << endl;
			}
			//getsucess = true;		
			
		}
		else
		{
			mysql_close(&myCont);
			mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
			cout << "���͵�Դ����ָ��ʧ�ܣ��豸û���ϴ���¼���ò���socket��" << endl;
			return -22;
		}
		mysql_free_result(result); //�ͷŻ��棬�ر�ע�⣬���ͷŻᵼ���ڴ�����
	}
    

	if (0 == allow_alarm.compare("1"))
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		root["deviceopen"] = Json::Value("setdeviceopen");
		root["card"] = Json::Value(str_card);
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring	
		send(ClientS, str.c_str(), str.length(), 0);  // ������Ϣ
		//cout << "���Ϳ�����Դָ�" << endl;
	}
	else
	{
		Json::Value root;             // ��ʾ���� json ����
		root["errno"] = Json::Value(0);     // �½�һ�� Key����Ϊ��key_string���������ַ���ֵ��"value_string"��
		root["error"] = Json::Value("sucess"); // �½�һ�� Key����Ϊ��key_number����������ֵ��12345��	
		root["deviceopen"] = Json::Value("setdeviceclose");
		root["card"] = Json::Value(str_card);
		Json::FastWriter  fast_writer;//�鿴json���ݶ���
		string str = fast_writer.write(root); //jsonתstring	
		send(ClientS, str.c_str(), str.length(), 0);  // ������Ϣ
		//cout << "���͹رյ�Դָ�" << endl;
	}

	{
		m_strToken = "DELETE    FROM  set_device_open   WHERE card = '" + str_card + "' ORDER BY time ASC LIMIT 1 ";
		res = mysql_query(&myCont, (const  char *)m_strToken.c_str()); //ִ��SQL���,ͨ��token����username
		mysql_close(&myCont);
		mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
		if (!res)
		{
			return 0;

		}
		else
			return -2;
	}
	mysql_close(&myCont);
	mysql_library_end();//���ǵ��� mysql_close ֮����� mysql_library_end() ���ͷ�δ���ͷŵ��ڴ�
	return -123;

}

