1、增加异常处理try catch
2、增加日志
3、定义接口,文件存放目录及是否需要权限问题
4、ie不兼容需要找人改写



1、增加配置
nfs挂载目录
文件名字有重复时是覆盖写还是追加写
try catch
更改root用户为websocket不可行，设置nfs写文件为nfsnobody用户

2、日志

client:{"filename":"1332.png","size":302779,"type":"base64","parameters":[]}
server:
"{\"type\":\"STOR\",\"message\":\"Upload initialized. Wait for data\",\"code\": 200}"

"{\"type\":\"STOR\",\"message\":\"init file failed\",\"code\": 500}";

client:发送文件内容,当内容发送完时关闭连接
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