1�������쳣����try catch
2��������־
3������ӿ�,�ļ����Ŀ¼���Ƿ���ҪȨ������
4��ie��������Ҫ���˸�д



1����������
nfs����Ŀ¼
�ļ��������ظ�ʱ�Ǹ���д����׷��д
try catch
����root�û�Ϊwebsocket�����У�����nfsд�ļ�Ϊnfsnobody�û�

2����־

client:{"filename":"1332.png","size":302779,"type":"base64","parameters":[]}
server:
"{\"type\":\"STOR\",\"message\":\"Upload initialized. Wait for data\",\"code\": 200}"

"{\"type\":\"STOR\",\"message\":\"init file failed\",\"code\": 500}";

client:�����ļ�����,�����ݷ�����ʱ�ر�����
server:
"{\"type\":\"DATA\",\"code\": 200,\"bytesRead\":1024}";

"{\"type\":\"DATA\",\"code\": 500,\"bytesRead\":0}";


{
   "action":"DELETE_FILES",
"transfer":"base64",
   "data":{
      "files":[{
         "file":"/test.pdf"
},{
         "file":"/test.png"
},{
         "file":"/test.jpeg"
}],
"deleteDir":false
   },
   "ts":"2015-07-31 08:00:10"
}

{"action":"RENAME_FILE","transfer":"base64","data":{
			  				"oldFile":"/vs_community.exe","newFile" : "/vs.exe"},
													"ts" : "2015 - 07 - 31 08 : 00 : 30"}

ws://172.18.100.72:8080/upload
ws://172.18.100.72:8080/rename
ws://172.18.100.72:8080/delete
ws://172.18.100.72:8080/list