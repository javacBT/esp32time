#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

//热点
const char* AP_SSID  = "ESP32_Config"; //热点名称
const char* AP_PASS  = "12345678";  //密码

String ssid = "";
String pass = "";

#define ROOT_HTML  "<!DOCTYPE html><html><head><title>WIFI Config by lwang</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><style type=\"text/css\">.input{display: block; margin-top: 10px;}.input span{width: 100px; float: left; float: left; height: 36px; line-height: 36px;}.input input{height: 30px;width: 200px;}.btn{width: 120px; height: 35px; background-color: #000000; border:0px; color:#ffffff; margin-top:15px; margin-left:100px;}</style><body><form method=\"GET\" action=\"connect\"><label class=\"input\"><span>WiFi SSID</span><input type=\"text\" name=\"ssid\"></label><label class=\"input\"><span>WiFi PASS</span><input type=\"text\"  name=\"pass\"></label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"Submie\"></form></body></html>"
WebServer server(80);

#define PIN        4

#define NUMPIXELS 256

#define DELAYVAL 1000 * 20

int lamps[32][8]; //因为8*32灯是蛇形排列 所以要转换一下

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

HTTPClient http;
DynamicJsonDocument doc(1024);

String wea = ""; //天气
int tem = 0; //温度
int sD = 0; //湿度

void setup() {

  //串口日志绑定
  Serial.begin(9600);

  //初始化led灯
  initialization();

  //读取或者保存wifi信息 
  getwifi();

  //http请求接口 获取天气使用
  http.begin("http://122.51.51.37:8286/weather/get_weather");

  //连接网络
  connectWifi(ssid,pass);
  

}

int showJudge = 1;
void loop() {

  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) { //WIFI已连接

   if(showJudge==1){

    showWeather();
    showJudge = 2;
    
   }else if(showJudge==2){

    humidity();
    showJudge = 3;
    
   }else {

    showJudge = 1;
    timeDisplay();
   }
  
   delay(DELAYVAL);
  }

}

//初始化led灯
void initialization(){

    pixels.begin();

          for(int x=1;x<=32;x++){
            for(int y=1;y<=8;y++){

                if(x%2==0){

                    lamps[x-1][y-1]=(x*8)-(y-1) -1;

                }else{

                    lamps[x-1][y-1]=(x-1)*8+y -1;
                }
            }
        }
}

//创建热点启动http服务用来设置wifi
boolean hotspotJudge = false; //是否设置过wifi
void hotspot(){

  //无wifi图标
  noWifilogo();

  //创建热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASS);

  Serial.println(WiFi.softAPIP());

  //192.168.4.1 首页
  server.on("/", []() {
    server.send(200, "text/html", ROOT_HTML);
  });

  //192.168.4.1 设置wifi
  server.on("/connect", [](){
    
    server.send(200, "text/html", "<html><body><h1>successd,conning...</h1></body></html>");
    ssid = server.arg("ssid");
    pass = server.arg("pass");
    hotspotJudge = true;
    connectWifi(ssid,pass);

  });

  server.begin();
}

//连接wifi
void connectWifi(String ssid,String pass){

    connectlogo();

    int connectWifiIndex = 1;
    
    WiFi.softAPdisconnect(true);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    while (WiFi.status() != WL_CONNECTED) { 
      delay(1000);
      connectWifiIndex = connectWifiIndex + 1;

      //等待五秒无网络跳出
      if(connectWifiIndex==5)break;
    }
    
    if(WiFi.status() == WL_CONNECTED){

      //保存wifi信息 
      if(hotspotJudge)setwifi();
      //联网成功后获取一次天气信息
      httpGet();
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      
    }else{

      hotspot();
    }
}

//读取wifi等信息 断电保存
void getwifi(){

  EEPROM.begin(2048);

  ssid=get_string(EEPROM.read(10),15);
  pass=get_string(EEPROM.read(50),55);
}

//保存WiFi信息
void setwifi(){

    set_string(10,15,ssid,1); //保存wifi名称
    set_string(50,55,pass,1); //保存wifi名称
}

//用EEPROM的a位保存字符串的长度，字符串的从EEPROM的b位开始保存，str为要保存的字符串，s为是否保存字符串长度
void set_string(int a,int b,String str,int s)
{
  if(s)EEPROM.write(a,str.length()); //EEPROM第a位，写入str字符串的长度
  //通过一个for循环，把str所有数据，逐个保存在EEPROM
  for(int i = 0; i < str.length(); i++){
    EEPROM.write(b + i, str[i]);
  }
  EEPROM.commit();  //执行保存EEPROM
  
}
 
//获取指定EEPROM位置的字符串，a是字符串长度，b是起始位，从EEPROM的b位开始读取
String get_string(int a,int b){
  String data="";
  //通过一个for循环，从EEPROM中逐个取出每一位的值，并连接起来
  for(int i=0; i<a; i++){
    data += char(EEPROM.read(b+i));
  }
  return data;
}

//http请求获取天气
void httpGet(){

  int httpCode = http.GET();

  if (httpCode > 0){

    String payload = http.getString();
    doc.clear();
    deserializeJson(doc,payload);
    JsonObject obj = doc.as<JsonObject>();
    String httpWea = obj["wea"];
    wea = httpWea;
    tem = (int)obj["tem"];
    tem = tem%100;
    sD = (int)obj["sD"];
    sD = sD%100;
  }

  http.end();
}

//获取时间
int H=0;
void timeDisplay(){

  time_t now ;
  struct tm *tm_now ;
  time(&now) ;
  tm_now = localtime(&now) ;
  
  int hour = tm_now->tm_hour; //时
  int mins = tm_now->tm_min;  //分
  int wday = tm_now->tm_wday; //周

  //每小时请求一次天气
  if(H!=hour){

      H = hour;
      httpGet();
  }

  pixels.clear();
  timelogo();

  Numbr(hour/10,11,1);
  Numbr(hour%10,15,1);
  Numbr(-1,19,1);
  Numbr(mins/10,21,1);
  Numbr(mins%10,25,1);

  wdayDisplay(wday);
}

//显示周几
void wdayDisplay(int wday){

  setColor(lamps[11][7],255,100,6);
  setColor(lamps[12][7],255,100,6);

  setColor(lamps[14][7],255,100,6);
  setColor(lamps[15][7],255,100,6);

  setColor(lamps[17][7],255,100,6);
  setColor(lamps[18][7],255,100,6);

  setColor(lamps[20][7],255,100,6);
  setColor(lamps[21][7],255,100,6);

  setColor(lamps[23][7],255,100,6);
  setColor(lamps[24][7],255,100,6);

  setColor(lamps[26][7],255,100,6);
  setColor(lamps[27][7],255,100,6);

  setColor(lamps[29][7],255,100,6);
  setColor(lamps[30][7],255,100,6);

  switch(wday){

      case 1:
        setColor(lamps[11][7],219,0,5);
        setColor(lamps[12][7],219,0,5);
        break;
      case 2:
        setColor(lamps[14][7],219,0,5);
        setColor(lamps[15][7],219,0,5);
        break;
      case 3:
        setColor(lamps[17][7],219,0,5);
        setColor(lamps[18][7],219,0,5);
        break;
      case 4:
        setColor(lamps[20][7],219,0,5);
        setColor(lamps[21][7],219,0,5);
        break;
      case 5:
        setColor(lamps[23][7],219,0,5);
        setColor(lamps[24][7],219,0,5);
        break;
      case 6:
        setColor(lamps[26][7],219,0,5);
        setColor(lamps[27][7],219,0,5);
        break;
      case 0:
        setColor(lamps[29][7],219,0,5);
        setColor(lamps[30][7],219,0,5);
        break;
  }
}

//显示温度
void showWeather(){

  pixels.clear();

  //气温
  int tem1 = tem/10;
  int tem2 = tem%10;

  if(tem<0){

    setColor(lamps[11][4],153,217,234);
    setColor(lamps[12][4],153,217,234);
    setColor(lamps[13][4],153,217,234);

    Numbr(tem1,14,2);
    Numbr(tem2,18,2);
    
    setColor(lamps[23][2],153,217,234);
    setColor(lamps[24][2],153,217,234);
    setColor(lamps[25][2],153,217,234);
    
    setColor(lamps[23][3],153,217,234);
    setColor(lamps[25][3],153,217,234);
    
    setColor(lamps[23][4],153,217,234);
    
    setColor(lamps[23][5],153,217,234);
    setColor(lamps[25][5],153,217,234);
    
    setColor(lamps[23][6],153,217,234);
    setColor(lamps[24][6],153,217,234);
    setColor(lamps[25][6],153,217,234);
    
    setColor(lamps[27][2],153,217,234);
    
  }else{

    Numbr(tem1,11,2);
    Numbr(tem2,15,2);
    
    setColor(lamps[20][2],153,217,234);
    setColor(lamps[21][2],153,217,234);
    setColor(lamps[22][2],153,217,234);
    
    setColor(lamps[20][3],153,217,234);
    setColor(lamps[22][3],153,217,234);
    
    setColor(lamps[20][4],153,217,234);
    
    setColor(lamps[20][5],153,217,234);
    setColor(lamps[22][5],153,217,234);
    
    setColor(lamps[20][6],153,217,234);
    setColor(lamps[21][6],153,217,234);
    setColor(lamps[22][6],153,217,234);
    
    setColor(lamps[24][2],153,217,234);
  }

  //天气
  if(wea=="雨"){

   setColor(lamps[7][1],195,195,195); 
   setColor(lamps[8][1],195,195,195); 

   setColor(lamps[3][2],195,195,195); 
   setColor(lamps[4][2],195,195,195); 
   setColor(lamps[6][2],195,195,195); 
   setColor(lamps[7][2],243,222,243); 
   setColor(lamps[8][2],243,222,243); 
   setColor(lamps[9][2],195,195,195); 

   setColor(lamps[1][3],127,127,127);
   setColor(lamps[2][3],195,195,195);
   setColor(lamps[3][3],243,222,243);
   setColor(lamps[4][3],243,222,243);
   setColor(lamps[5][3],195,195,195);
   setColor(lamps[6][3],243,222,243);
   setColor(lamps[7][3],243,222,243);
   setColor(lamps[8][3],243,222,243);
   setColor(lamps[9][3],243,222,243);
   setColor(lamps[10][3],195,195,195);

   setColor(lamps[2][4],127,127,127);
   setColor(lamps[3][4],195,195,195);
   setColor(lamps[4][4],195,195,195);
   setColor(lamps[5][4],195,195,195);
   setColor(lamps[6][4],195,195,195);
   setColor(lamps[7][4],195,195,195);
   setColor(lamps[8][4],195,195,195);
   
   setColor(lamps[9][4],0,162,232);
   setColor(lamps[3][5],0,162,232);
   setColor(lamps[9][5],49,193,255);
   setColor(lamps[3][6],49,193,255);
   setColor(lamps[6][6],0,162,232);
   
  }else if(wea=="雪"){

   setColor(lamps[7][1],195,195,195); 
   setColor(lamps[8][1],195,195,195); 

   setColor(lamps[3][2],195,195,195); 
   setColor(lamps[4][2],195,195,195); 
   setColor(lamps[6][2],195,195,195); 
   setColor(lamps[7][2],243,222,243); 
   setColor(lamps[8][2],243,222,243); 
   setColor(lamps[9][2],195,195,195); 

   setColor(lamps[1][3],127,127,127);
   setColor(lamps[2][3],195,195,195);
   setColor(lamps[3][3],243,222,243);
   setColor(lamps[4][3],243,222,243);
   setColor(lamps[5][3],195,195,195);
   setColor(lamps[6][3],243,222,243);
   setColor(lamps[7][3],243,222,243);
   setColor(lamps[8][3],243,222,243);
   setColor(lamps[9][3],243,222,243);
   setColor(lamps[10][3],195,195,195);

   setColor(lamps[2][4],127,127,127);
   setColor(lamps[3][4],195,195,195);
   setColor(lamps[4][4],195,195,195);
   setColor(lamps[5][4],195,195,195);
   setColor(lamps[6][4],195,195,195);
   setColor(lamps[7][4],195,195,195);
   setColor(lamps[8][4],195,195,195);
   
   setColor(lamps[9][4],239,94,98);
   setColor(lamps[3][5],239,94,98);
   setColor(lamps[9][5],239,94,98);
   setColor(lamps[3][6],239,94,98);
   setColor(lamps[6][6],239,94,98);
   
  }else if(wea=="阴"){

   setColor(lamps[7][1],239,94,98); 
   setColor(lamps[8][1],239,94,98); 

   setColor(lamps[3][2],239,94,98); 
   setColor(lamps[4][2],239,94,98); 
   setColor(lamps[6][2],239,94,98); 
   setColor(lamps[7][2],239,94,98); 
   setColor(lamps[8][2],239,94,98); 
   setColor(lamps[9][2],239,94,98); 

   setColor(lamps[1][3],239,94,98);
   setColor(lamps[2][3],239,94,98);
   setColor(lamps[3][3],239,94,98);
   setColor(lamps[4][3],239,94,98);
   setColor(lamps[5][3],239,94,98);
   setColor(lamps[6][3],239,94,98);
   setColor(lamps[7][3],239,94,98);
   setColor(lamps[8][3],239,94,98);
   setColor(lamps[9][3],239,94,98);
   setColor(lamps[10][3],239,94,98);

   setColor(lamps[2][4],239,94,98);
   setColor(lamps[3][4],239,94,98);
   setColor(lamps[4][4],239,94,98);
   setColor(lamps[5][4],239,94,98);
   setColor(lamps[6][4],239,94,98);
   setColor(lamps[7][4],239,94,98);
   setColor(lamps[8][4],239,94,98);
   
  }else{ //晴

    setColor(lamps[2][0],255,100,6);
    setColor(lamps[5][0],255,100,6);

    setColor(lamps[2][2],255,100,6);
    setColor(lamps[3][2],255,100,6);
    setColor(lamps[4][2],255,100,6);
    setColor(lamps[5][2],255,100,6);
    setColor(lamps[2][3],255,100,6);
    setColor(lamps[3][3],255,100,6);
    setColor(lamps[4][3],255,100,6);
    setColor(lamps[5][3],255,100,6);
    setColor(lamps[2][4],255,100,6);
    setColor(lamps[3][4],255,100,6);
    setColor(lamps[4][4],255,100,6);
    setColor(lamps[5][4],255,100,6);
    setColor(lamps[2][5],255,100,6);
    setColor(lamps[3][5],255,100,6);
    setColor(lamps[4][5],255,100,6);
    setColor(lamps[5][5],255,100,6);

    setColor(lamps[2][7],255,100,6);
    setColor(lamps[5][7],255,100,6);

    setColor(lamps[0][2],255,100,6);
    setColor(lamps[0][5],255,100,6);

    setColor(lamps[7][2],255,100,6);
    setColor(lamps[7][5],255,100,6);
  }
}

//湿度显示
void humidity(){

  pixels.clear();

  //气温
  int sD1 = sD/10;
  int sD2 = sD%10;

    Numbr(sD1,11,2);
    Numbr(sD2,15,2);
    
    setColor(lamps[20][2],153,217,234);
    setColor(lamps[22][2],153,217,234);
    setColor(lamps[22][3],153,217,234);
    setColor(lamps[21][4],153,217,234);
    setColor(lamps[20][5],153,217,234);
    setColor(lamps[20][6],153,217,234);
    setColor(lamps[22][6],153,217,234);


    

    setColor(lamps[4][0],153,217,234);
    
    setColor(lamps[4][1],153,217,234);

    setColor(lamps[3][2],239,94,98);
    setColor(lamps[4][2],239,94,98);
    setColor(lamps[5][2],153,217,234);

    setColor(lamps[3][3],239,94,98);
    setColor(lamps[4][3],153,217,234);
    setColor(lamps[5][3],153,217,234);

    setColor(lamps[2][4],239,94,98);
    setColor(lamps[3][4],153,217,234);
    setColor(lamps[4][4],153,217,234);
    setColor(lamps[5][4],153,217,234);
    setColor(lamps[6][4],0,162,232);
    
    setColor(lamps[2][5],153,217,234);
    setColor(lamps[3][5],153,217,234);
    setColor(lamps[4][5],153,217,234);
    setColor(lamps[5][5],153,217,234);
    setColor(lamps[6][5],0,162,232);
    
    setColor(lamps[2][6],153,217,234);
    setColor(lamps[3][6],153,217,234);
    setColor(lamps[4][6],153,217,234);
    setColor(lamps[5][6],0,162,232);
    setColor(lamps[6][6],0,162,232);
    
    setColor(lamps[3][7],0,162,232);
    setColor(lamps[4][7],0,162,232);
    setColor(lamps[5][7],0,162,232);
    
}

//数值 显示位置
void Numbr(int num,int x,int y){

  if(x>=32 || y>=8)return;

  switch(num){

    case -1:
      setColor(lamps[x+1][y+1],153,217,234);
      setColor(lamps[x+1][y+3],153,217,234);
      break;
    case 0:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],147,52,252);
      setColor(lamps[x+3][y],147,52,252);
      setColor(lamps[x+1][y+1],153,217,234);
      setColor(lamps[x+3][y+1],147,52,252);
      setColor(lamps[x+1][y+2],0,162,232);
      setColor(lamps[x+3][y+2],85,204,255);
      setColor(lamps[x+1][y+3],153,217,234);
      setColor(lamps[x+3][y+3],153,217,234);
      setColor(lamps[x+1][y+4],153,217,234);
      setColor(lamps[x+2][y+4],147,52,252);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 1:
      setColor(lamps[x+3][y],85,204,255);
      setColor(lamps[x+2][y+1],147,52,252);
      setColor(lamps[x+3][y+1],153,217,234);
      setColor(lamps[x+3][y+2],147,52,252);
      setColor(lamps[x+3][y+3],85,204,255);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 2:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],147,52,252);
      setColor(lamps[x+3][y],147,52,252);
      setColor(lamps[x+3][y+1],147,52,252);
      setColor(lamps[x+1][y+2],0,162,232);
      setColor(lamps[x+2][y+2],85,204,255);
      setColor(lamps[x+3][y+2],85,204,255);
      setColor(lamps[x+1][y+3],153,217,234);
      setColor(lamps[x+1][y+4],153,217,234);
      setColor(lamps[x+2][y+4],147,52,252);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 3:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],147,52,252);
      setColor(lamps[x+3][y],147,52,252);
      setColor(lamps[x+3][y+1],147,52,252);
      setColor(lamps[x+1][y+2],0,162,232);
      setColor(lamps[x+2][y+2],85,204,255);
      setColor(lamps[x+3][y+2],85,204,255);
      setColor(lamps[x+3][y+3],147,52,252);
      setColor(lamps[x+1][y+4],147,52,252);
      setColor(lamps[x+2][y+4],147,52,252);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 4:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+3][y],147,52,252);
      setColor(lamps[x+1][y+1],153,217,234);
      setColor(lamps[x+3][y+1],147,52,252);
      setColor(lamps[x+1][y+2],147,52,252);
      setColor(lamps[x+2][y+2],85,204,255);
      setColor(lamps[x+3][y+2],85,204,255);
      setColor(lamps[x+3][y+3],147,52,252);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 5:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],153,217,234);
      setColor(lamps[x+3][y],153,217,234);
      setColor(lamps[x+1][y+1],0,162,232);
      setColor(lamps[x+1][y+2],0,162,232);
      setColor(lamps[x+2][y+2],147,52,252);
      setColor(lamps[x+3][y+2],147,52,252);
      setColor(lamps[x+3][y+3],147,52,252);
      setColor(lamps[x+1][y+4],147,52,252);
      setColor(lamps[x+2][y+4],147,52,252);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 6:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],153,217,234);
      setColor(lamps[x+3][y],153,217,234);
      setColor(lamps[x+1][y+1],0,162,232);
      setColor(lamps[x+1][y+2],0,162,232);
      setColor(lamps[x+2][y+2],147,52,252);
      setColor(lamps[x+3][y+2],147,52,252);
      setColor(lamps[x+1][y+3],147,52,252);
      setColor(lamps[x+3][y+3],147,52,252);
      setColor(lamps[x+1][y+4],147,52,252);
      setColor(lamps[x+2][y+4],147,52,252);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 7:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],0,162,232);
      setColor(lamps[x+3][y],153,217,234);
      setColor(lamps[x+3][y+1],153,217,234);
      setColor(lamps[x+3][y+2],147,52,252);
      setColor(lamps[x+3][y+3],147,52,252);
      setColor(lamps[x+3][y+4],85,204,255);
      break;
    case 8:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],0,162,232);
      setColor(lamps[x+3][y],153,217,234);
      setColor(lamps[x+1][y+1],147,52,252);
      setColor(lamps[x+3][y+1],153,217,234);
      setColor(lamps[x+1][y+2],153,217,234);
      setColor(lamps[x+2][y+2],147,52,252);
      setColor(lamps[x+3][y+2],147,52,252);
      setColor(lamps[x+1][y+3],147,52,252);
      setColor(lamps[x+3][y+3],147,52,252);
      setColor(lamps[x+1][y+4],147,52,252);
      setColor(lamps[x+2][y+4],147,52,252);
      setColor(lamps[x+3][y+4],0,162,232);
      break;
    case 9:
      setColor(lamps[x+1][y],0,162,232);
      setColor(lamps[x+2][y],0,162,232);
      setColor(lamps[x+3][y],153,217,234);
      setColor(lamps[x+1][y+1],147,52,252);
      setColor(lamps[x+3][y+1],153,217,234);
      setColor(lamps[x+1][y+2],153,217,234);
      setColor(lamps[x+2][y+2],147,52,252);
      setColor(lamps[x+3][y+2],147,52,252);
      setColor(lamps[x+3][y+3],147,52,252);
      setColor(lamps[x+3][y+4],0,162,232);
      break;
  }
}

//网络连接中
void connectlogo(){

    pixels.clear();

    setColor(lamps[11][4],0,162,232);

    setColor(lamps[13][4],0,162,232);

    setColor(lamps[15][4],0,162,232);

    setColor(lamps[17][4],0,162,232);

    setColor(lamps[19][4],0,162,232);
}

//无网络显示
void noWifilogo(){

    pixels.clear();

    setColor(lamps[13][3],0,162,232);
    setColor(lamps[13][4],0,162,232);

    setColor(lamps[15][2],0,162,232);
    setColor(lamps[15][3],0,162,232);
    setColor(lamps[15][4],0,162,232);
    setColor(lamps[15][5],0,162,232);

    setColor(lamps[17][3],0,162,232);
    setColor(lamps[17][4],0,162,232);
    
}

//显示logo
int logoindex = 1;
void timelogo(){

  switch(logoindex){

    case 1: //红心
    
      setColor(lamps[3][1],255,47,52);
      setColor(lamps[4][1],219,0,5);
      setColor(lamps[6][1],219,0,5);
      setColor(lamps[7][1],219,0,5);

      setColor(lamps[2][2],255,47,52);
      setColor(lamps[3][2],219,0,5);
      setColor(lamps[4][2],219,0,5);
      setColor(lamps[5][2],219,0,5);
      setColor(lamps[6][2],219,0,5);
      setColor(lamps[7][2],219,0,5);
      setColor(lamps[8][2],219,0,5);
      
      setColor(lamps[2][3],255,47,52);
      setColor(lamps[3][3],255,47,52);
      setColor(lamps[4][3],219,0,5);
      setColor(lamps[5][3],255,47,52);
      setColor(lamps[6][3],219,0,5);
      setColor(lamps[7][3],255,47,52);
      setColor(lamps[8][3],219,0,5);
      
      setColor(lamps[3][4],255,47,52);
      setColor(lamps[4][4],255,47,52);
      setColor(lamps[5][4],219,0,5);
      setColor(lamps[6][4],255,47,52);
      setColor(lamps[7][4],219,0,5);

      setColor(lamps[4][5],255,47,52);
      setColor(lamps[5][5],255,47,52);
      setColor(lamps[6][5],219,0,5);

      setColor(lamps[5][6],255,47,52);
      
      break;
      
    case 2: //鸭子

      setColor(lamps[6][1],255,201,14);
      setColor(lamps[7][1],255,201,14);
      setColor(lamps[8][1],255,201,14);
            
      setColor(lamps[5][2],255,201,14);
      setColor(lamps[6][2],255,201,14);
      setColor(lamps[7][2],0,0,0);
      setColor(lamps[8][2],255,201,14);

      setColor(lamps[5][3],255,201,14);
      setColor(lamps[6][3],239,228,176);
      setColor(lamps[7][3],255,201,14);
      setColor(lamps[8][3],237,28,36);
      setColor(lamps[9][3],237,28,36);
      setColor(lamps[10][3],237,28,36);

      setColor(lamps[2][4],239,228,176);
      setColor(lamps[4][4],255,201,14);
      setColor(lamps[5][4],255,201,14);
      setColor(lamps[6][4],255,201,14);
      setColor(lamps[7][4],255,201,14);
      setColor(lamps[8][4],237,28,36);
      setColor(lamps[9][4],237,28,36);
      setColor(lamps[10][4],237,28,36);

      setColor(lamps[2][5],255,201,14);
      setColor(lamps[3][5],255,201,14);
      setColor(lamps[4][5],239,228,176);
      setColor(lamps[5][5],255,201,14);
      setColor(lamps[6][5],255,201,14);
      setColor(lamps[7][5],255,201,14);
      setColor(lamps[8][5],239,228,176);
      setColor(lamps[9][5],255,201,14);
      
      setColor(lamps[3][6],255,201,14);
      setColor(lamps[4][6],255,201,14);
      setColor(lamps[5][6],255,201,14);
      setColor(lamps[6][6],255,201,14);
      setColor(lamps[7][6],239,228,176);
      setColor(lamps[8][6],255,201,14);
      
      break;
      
    case 3: //机器人
    
      setColor(lamps[3][1],0,162,232);
      setColor(lamps[4][1],0,162,232);
      setColor(lamps[5][1],0,162,232);
      setColor(lamps[6][1],0,162,232);
      setColor(lamps[7][1],0,162,232);
      setColor(lamps[8][1],0,162,232);
      
      setColor(lamps[2][2],0,162,232);
      setColor(lamps[9][2],0,162,232);


      setColor(lamps[1][3],64,198,255);
      setColor(lamps[2][3],0,162,232);
      setColor(lamps[4][3],0,162,232);
      setColor(lamps[7][3],0,162,232);
      setColor(lamps[9][3],0,162,232);
      setColor(lamps[10][3],64,198,255);

      setColor(lamps[1][4],64,198,255);
      setColor(lamps[2][4],0,162,232);
      setColor(lamps[4][4],0,162,232);
      setColor(lamps[7][4],0,162,232);
      setColor(lamps[9][4],0,162,232);
      setColor(lamps[10][4],64,198,255);    
        
      setColor(lamps[2][5],0,162,232);
      setColor(lamps[9][5],0,162,232);
      
      setColor(lamps[3][6],160,55,238);
      setColor(lamps[4][6],0,162,232);
      setColor(lamps[5][6],160,55,238);
      setColor(lamps[6][6],160,55,238);
      setColor(lamps[7][6],0,162,232);
      setColor(lamps[8][6],160,55,238);
      
      break;
      
    case 4: //卡通人物
    
      setColor(lamps[3][1],237,28,36);
      setColor(lamps[4][1],237,28,36);

      setColor(lamps[1][2],237,28,36);
      setColor(lamps[2][2],237,28,36);
      setColor(lamps[3][2],237,28,36);
      setColor(lamps[4][2],237,28,36);
      setColor(lamps[5][2],237,28,36);
      setColor(lamps[6][2],237,28,36);
      setColor(lamps[7][2],237,28,36);
      
      setColor(lamps[1][3],237,28,36);
      setColor(lamps[2][3],237,28,36);
      setColor(lamps[3][3],237,28,36);
      setColor(lamps[4][3],237,28,36);
      setColor(lamps[5][3],237,28,36);
      setColor(lamps[6][3],237,28,36);
      setColor(lamps[7][3],237,28,36);
      setColor(lamps[8][3],237,28,36);
      
      setColor(lamps[1][4],243,207,175);
      setColor(lamps[2][4],243,207,175);
      setColor(lamps[3][4],243,207,175);
      setColor(lamps[4][4],243,207,175);
      setColor(lamps[5][4],0,0,0);
      setColor(lamps[6][4],243,207,175);
      
      setColor(lamps[1][5],243,207,175);
      setColor(lamps[2][5],243,207,175);
      setColor(lamps[3][5],243,207,175);
      setColor(lamps[4][5],243,207,175);
      setColor(lamps[5][5],243,207,175);
      setColor(lamps[6][5],243,207,175);
      setColor(lamps[7][5],243,207,175);
      setColor(lamps[8][5],243,103,111);
      
      setColor(lamps[2][6],243,207,175);
      setColor(lamps[3][6],243,207,175);
      setColor(lamps[4][6],237,217,99);
      setColor(lamps[5][6],237,217,99);

      break;
  }

  logoindex = logoindex + 1;

  if(logoindex==5)logoindex=1;
}

//亮灯
void setColor(int n,int r,int g,int b){

   pixels.setPixelColor(n, r, g, b,100);
   pixels.show();
}
