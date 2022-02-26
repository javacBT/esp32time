#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

//热点
const char* AP_SSID  = "ESP32_Config"; //热点名称
const char* AP_PASS  = "12345678";  //密码

#define ROOT_HTML  "<!DOCTYPE html><html><head><title>WIFI Config by lwang</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><style type=\"text/css\">.input{display: block; margin-top: 10px;}.input span{width: 100px; float: left; float: left; height: 36px; line-height: 36px;}.input input{height: 30px;width: 200px;}.btn{width: 120px; height: 35px; background-color: #000000; border:0px; color:#ffffff; margin-top:15px; margin-left:100px;}</style><body><form method=\"GET\" action=\"connect\"><label class=\"input\"><span>WiFi SSID</span><input type=\"text\" name=\"ssid\"></label><label class=\"input\"><span>WiFi PASS</span><input type=\"text\"  name=\"pass\"></label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"Submie\"></form></body></html>"
WebServer server(80);

#define PIN        4

#define NUMPIXELS 256

#define DELAYVAL 1000 * 60

int lamps[32][8]; //因为8*32灯是蛇形排列 所以要转换一下

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

void setup() {

  //初始化led的灯
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

  //串口日志绑定
  Serial.begin(9600);

  //无wifi图标
  noWifi();
  //创建热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASS);

  Serial.println(WiFi.softAPIP());

  //首页
  server.on("/", []() {
    server.send(200, "text/html", ROOT_HTML);
  });

  //连接
  server.on("/connect", [](){
    
    server.send(200, "text/html", "<html><body><h1>successd,conning...</h1></body></html>");
    WiFi.softAPdisconnect(true);
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    Serial.println(ssid);
    Serial.println(pass);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    while (WiFi.status() != WL_CONNECTED) { 
      delay(1000);
      Serial.println("连接中");
    }
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  });

  server.begin();
}

void loop() {

  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) { //WIFI已连接
    
   timeDisplay();

   logo();
  
   delay(DELAYVAL);
  }

}


//获取时间
void timeDisplay(){

  time_t now ;
  struct tm *tm_now ;
  time(&now) ;
  tm_now = localtime(&now) ;
  
  int hour = tm_now->tm_hour;
  int mins = tm_now->tm_min;
  Serial.println(hour);
  Serial.println(mins);

  pixels.clear();

  Numbr(hour/10,11,2);
  Numbr(hour%10,15,2);
  Numbr(-1,19,2);
  Numbr(mins/10,21,2);
  Numbr(mins%10,25,2);
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

//无网络显示
void noWifi(){

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
void logo(){

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

void setColor(int n,int r,int g,int b){

   pixels.setPixelColor(n, r, g, b,100);
   pixels.show();
}
