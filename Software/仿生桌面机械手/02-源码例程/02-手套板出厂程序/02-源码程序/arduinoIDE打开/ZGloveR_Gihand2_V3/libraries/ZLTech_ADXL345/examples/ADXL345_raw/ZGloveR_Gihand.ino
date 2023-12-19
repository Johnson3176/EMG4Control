/*
    硬件IO分布：
    1、x轴 - A0 y轴 - A1
    2、电位器 A6 A5 A4 A3 A2

    使用前需校准：
    1、手指全部合并，按一下校准；
    2、手指全部张开，按一下校准；
    
    控制效果：
    1、五指合并，控制小车和云台
    2、五指非全部合并状态，控制每一个手指。
    
*/

#include <EEPROM.h>           //调用<EEPROM.h库

#define DWQ_NUM     5               //宏定义电位器数量
#define PIN_beep    3               //宏定义蜂鸣器引脚
#define PIN_nled    13              //宏定义工作指示灯引脚
#define PIN_KEY_J   5               //宏定义校准引脚
#define PIN_KEY_M   4               //宏定义模式引脚

#define ADC_MIN_ADDR    0       //握紧时采集电位器值起始地址
#define ADC_MAX_ADDR    30      //张手时采集电位器值起始地址
#define ADC_VERYFY      32      //校验标志
#define FLAG_VERYFY     0x38    //标志值
#define INT_DATA_TYPE  2     //数据类型,数据类型=几个字节就是几，相当于数据拆分成字节
#define JXB_PWM_RANGE      1000.0 //量程
#define JXB_PWM_STRAT  (500+(2000-JXB_PWM_RANGE)/2)     //机械臂PWM起始值
#define X_MIDDLE 340
#define Y_MIDDLE 340

#define beep_on() digitalWrite(PIN_beep, LOW);
#define beep_off() digitalWrite(PIN_beep, HIGH);

#define nled_on() digitalWrite(PIN_nled, LOW);
#define nled_off() digitalWrite(PIN_nled, HIGH);

String cmd_str = "";
char cmd_return[200], cmd_return_bak[200];
byte dwq_pin[DWQ_NUM] = {A6, A5, A4, A3, A2};               //定义电位器引脚数组
int  ADC_MAX[DWQ_NUM] = {0, 0, 0, 0, 0};  //默认握紧时采集电位器值
int  ADC_MIN[DWQ_NUM] = {0, 0, 0, 0, 0};  //默认张手时采集电位器值
int  ADC_MID[DWQ_NUM] = {0, 0, 0, 0, 0};            //默认张手时采集电位器值
int  pos[DWQ_NUM] = {0}, pos_x=0, pos_y=0;          //变量pos用来存储转化后的电位器数据
int  pos_bak[5] = {0};                              //变量pos备份值
int  pwm_value[DWQ_NUM] = {0};                      //变量pwm值
float pos_p[DWQ_NUM] = {1,1,1,1,1};                         //放大倍数
u8 robot_mode = 0;
u8 adc_veryfy = 0;
u32 adc_systick_ms = 0;

void setup(){
    Serial.begin(115200);                           //初始化波特率为115200
    pinMode(PIN_nled, OUTPUT);                      //设置LED引脚为输出模式
    pinMode(PIN_beep, OUTPUT);                      //设置蜂鸣器引脚为输出模式
    pinMode(PIN_KEY_M, INPUT_PULLUP);               //将模式按键对应引脚设置为内部上拉输入模式，防止误判
    pinMode(PIN_KEY_J, INPUT_PULLUP);               //将校准按键对应引脚设置为内部上拉输入模式，防止误判

    eepromRead(ADC_MIN_ADDR, sizeof(ADC_MIN), (u8 *)ADC_MIN);//读取存储电位器最小值
    eepromRead(ADC_MAX_ADDR, sizeof(ADC_MAX), (u8 *)ADC_MAX);//读取存储电位器最大值
    eepromRead(ADC_VERYFY, 1, &adc_veryfy);//读取校验值

    for(int i=0;i<DWQ_NUM;i++) {
        ADC_MID[i] = (ADC_MIN[i] + ADC_MAX[i])/2;
        pos_p[i] = JXB_PWM_RANGE/abs(ADC_MIN[i] - ADC_MAX[i]);//放大倍数
    }
    
    //启动示意
    nled_on();beep_on();delay(100);nled_off();beep_off();delay(100);
    nled_on();beep_on();delay(100);nled_off();beep_off();delay(100);
    nled_on();beep_on();delay(100);nled_off();beep_off();delay(100);

#if 0
      sprintf(cmd_return,"ADC_MIN:%d %d %d %d %d",
                          ADC_MIN[0], ADC_MIN[1], ADC_MIN[2], ADC_MIN[3], ADC_MIN[4]);
      Serial.println(cmd_return);  

      sprintf(cmd_return,"ADC_MAX:%d %d %d %d %d",
                          ADC_MAX[0], ADC_MAX[1], ADC_MAX[2], ADC_MAX[3], ADC_MAX[4]);
      Serial.println(cmd_return);   
#endif
}

void loop(){ 
    loop_nled();        //led灯闪烁函数
    loop_key_j();       //校验函数
    loop_key_m();       //模式选择
}

/*
    读写eeprom数据
    addr 地址
    len 数据长度
    mydat 数据buf
*/
void eepromRead(u32 addr, u8 len, u8 *mydat){
    for(u8 i = 0; i < len; i++) {//求取旋钮平均值，存储到EEPROM
        mydat[i] = EEPROM.read(addr+i);
    }        
}

/*
    读写eeprom数据
    addr 地址
    len 数据长度
    mydat 数据buf
*/
void eepromWrite(u32 addr, u8 len, u8 *mydat){
    for(u8 i = 0; i < len; i++) {//求取旋钮平均值，存储到EEPROM
        EEPROM.write(addr+i, mydat[i]);
    }        
}


int Filter(int FILTER_N, int pin) {
  int filter_buf[FILTER_N];
  int i, j;
  int filter_temp;
  for(i = 0; i < FILTER_N; i++) {
    filter_buf[i] = analogRead(pin);
    delayMicroseconds(300);
  }
  // 采样值从小到大排列（冒泡法）
  for(j = 0; j < FILTER_N - 1; j++) {
    for(i = 0; i < FILTER_N - 1 - j; i++) {
      if(filter_buf[i] > filter_buf[i + 1]) {
        filter_temp = filter_buf[i];
        filter_buf[i] = filter_buf[i + 1];
        filter_buf[i + 1] = filter_temp;
      }
    }
  }
  return filter_buf[(FILTER_N - 1) / 2];
}

void adc_read2buf() {
    static u32 xn_value;
    for(int i=0;i<5;i++) {
        xn_value = 0;
        for(int j=0;j<10;j++) {
            xn_value+=analogRead(dwq_pin[i]);
            delayMicroseconds(100);
         }
         xn_value = xn_value/10.0;
         pos[i] = xn_value;
    }

    pos_x = Filter(9, A0);
    pos_y = Filter(9, A1);

#if 0
      sprintf(cmd_return,"ad0:%d ad1:%d ad2:%d ad3:%d ad4:%d ad5:%d ad6:%d ",
                          pos_x, pos_y, pos[0], pos[1], pos[2], pos[3], pos[4]);
      Serial.println(cmd_return);  
      delay(100);
#endif
}

void loop_nled() {
    static u32 systick_ms_bak = 0;
    static u8 flag=0;
    if(millis() - systick_ms_bak<500)return;
    systick_ms_bak = millis();
    flag = !flag;
    if(flag) {
        nled_on();
    } else {
        nled_off();
    }
}

void loop_key_j() {
    static u8 flag=0;
    if((digitalRead(PIN_KEY_J)==LOW) && (flag==0)) {//第一次校准，握手值：在舒适的方式下最大程度的握紧手指，此时按下校准按钮
        delay(10);    
        if((digitalRead(PIN_KEY_J)==LOW) && (flag==0)){
            while(digitalRead(PIN_KEY_J)==LOW);
            adc_read2buf();
            for(int i=0;i<DWQ_NUM;i++) {
                ADC_MIN[i] = pos[i];
            }
            beep_on();delay(100);beep_off();
            eepromWrite(ADC_MIN_ADDR, sizeof(ADC_MIN), (u8 *)ADC_MIN);
            flag = 1; 

                sprintf(cmd_return,"ad0:%d ad1:%d ad2:%d ad3:%d ad4:%d ad5:%d ad6:%d ",
                pos_x-X_MIDDLE, pos_y-Y_MIDDLE, pos[0], pos[1], pos[2], pos[3], pos[4]);
                Serial.println(cmd_return);  
        }
    } else if((digitalRead(PIN_KEY_J)==LOW) && (flag==1)) {//第二次校准，伸手值：在舒适的方式下最大程度的伸开手指，此时按下校准按钮
        delay(10);    
        if((digitalRead(PIN_KEY_J)==LOW ) && (flag==1)){
            while(digitalRead(PIN_KEY_J)==LOW);
            adc_read2buf();
            for(int i=0;i<DWQ_NUM;i++) {
                ADC_MAX[i] = pos[i];
            }
            for(int i=0;i<DWQ_NUM;i++) {
                ADC_MID[i] = (ADC_MIN[i] + ADC_MAX[i])/2;
                pos_p[i] = 1.0*JXB_PWM_RANGE/(abs(ADC_MIN[i] - ADC_MAX[i]));//放大倍数
            }
            beep_on();delay(500);beep_off();
            eepromWrite(ADC_MAX_ADDR,  sizeof(ADC_MAX), (u8 *)ADC_MAX);
            adc_veryfy = FLAG_VERYFY;
            eepromWrite(ADC_VERYFY, 1, &adc_veryfy);//写入校验值校验值

            flag = 0;  

                sprintf(cmd_return,"ad0:%d ad1:%d ad2:%d ad3:%d ad4:%d ad5:%d ad6:%d ",
                pos_x-X_MIDDLE, pos_y-Y_MIDDLE, pos[0], pos[1], pos[2], pos[3], pos[4]);
                Serial.println(cmd_return);          
            }    
    }
    
    if(digitalRead(PIN_KEY_M)==LOW) {//模式切换按钮
        delay(10);    
        if(digitalRead(PIN_KEY_M)==LOW){
            robot_mode++;
        }

        if(robot_mode==2)robot_mode=0;//最多2种模式
        for(int i=0;i<robot_mode+1;i++) {
            beep_on();delay(300);beep_off();delay(100);
        }
    }    
}

void loop_key_m() {
    static bool beep_flag;
    if(adc_veryfy != FLAG_VERYFY){
        if(millis() - adc_systick_ms<500)return;
        adc_systick_ms = millis();
        if(beep_flag) {
            beep_on();
        } else {
            beep_off();
        }
        beep_flag = !beep_flag;
        return;
    }

    
    adc_read2buf();
    loop_Gihand_R();
}

/*
    控制效果：
    1、食指、中指、无名指、小指 四指张开的情况下，手套前倾小车向前运行，后倾向后运行，左倾向左运行，右倾向右运行；
    2、食指、中指、无名指、小指 四指闭合的情况下，手套前倾机械臂弯曲，后倾恢复直立，左倾云台左转，右倾云台右转；大拇指下压爪子闭合，大拇指上提爪子张开
*/
void loop_Gihand_R(){
    static u16 bak[6];
    static u32 systick_ms_bak = 0, xn_value;
    static u8 curMode=0;
    int left_speed, right_speed;
    if(millis() - systick_ms_bak > 50) {
      systick_ms_bak = millis();
      for(int i=0;i<5;i++) {
        //600 800 
          pwm_value[i] = (int)(1500+(pos[i]-ADC_MID[i])*pos_p[i]);
          if(abs(pos_bak[i] - pos[i]) >= 5) {
              pos_bak[i] = pos[i];
              
              if(pwm_value[i]>2500)pwm_value[i]=2500;
              else if(pwm_value[i]<500)pwm_value[i]=500;
          }

          //sprintf(cmd_return,"#000P%04dT0100!", (pos_x-X_MIDDLE)*10);
          //Serial.println(cmd_return); 
      }

      if((3000-pwm_value[0]>1900) &&
      (pwm_value[1]<1100) &&
      (pwm_value[2]<1100) &&
      (3000-pwm_value[3]>1900) &&
      (3000-pwm_value[4]>1900) 
      ) {
        //发生模式切换立即发送全部停止指令
        if(curMode != 0) {
            Serial.println("#255P1500T0000!");
            curMode = 0;
            return;
        }
        
        left_speed = 1500+(pos_y-Y_MIDDLE)*10+(pos_x-X_MIDDLE)*10;
        right_speed = 1500-(pos_y-Y_MIDDLE)*10+(pos_x-X_MIDDLE)*10;
        
        if(left_speed>2500)left_speed = 2500;
        if(right_speed>2500)right_speed = 2500;
        if(left_speed<500)left_speed = 500;
        if(right_speed<500)right_speed = 500;
        
        if(abs(left_speed-1500) < 200)left_speed=1500;
        if(abs(right_speed-1500) < 200)right_speed=1500;
        
        sprintf(cmd_return, "{#000P%04dT0100!#006P%04dT0000!#007P%04dT0000!}", 1500-(pos_x-X_MIDDLE)*10, left_speed, right_speed); 
        Serial.println(cmd_return);
        curMode = 0;
            
     } else {   
        //发生模式切换立即发送全部停止指令
        if(curMode != 1) {
            Serial.println("#255P1500T0000!");
            curMode = 1;
            return;
        }
        cmd_str = "{";
        for(int i=0;i<5;i++) {
            if(abs(bak[i]-pwm_value[i])>10){
                switch(i){
                    case 0:
                    sprintf(cmd_return, "#%03dP%04dT0100!",i,3000-pwm_value[i]); 
                    cmd_str = cmd_str+cmd_return;
                    break;
    
                    case 1:
                    sprintf(cmd_return, "#%03dP%04dT0100!",i,pwm_value[i]); 
                    cmd_str = cmd_str+cmd_return;
                    break;
    
                    case 2:
                    sprintf(cmd_return, "#%03dP%04dT0100!",i,pwm_value[i]); 
                    cmd_str = cmd_str+cmd_return;
                    break;
    
                    case 3:
                    sprintf(cmd_return, "#%03dP%04dT0100!",i,3000-pwm_value[i]); 
                    cmd_str = cmd_str+cmd_return;
                    break;
    
                    case 4:
                    sprintf(cmd_return, "#%03dP%04dT0100!",i,3000-pwm_value[i]); 
                    cmd_str = cmd_str+cmd_return;
                    break;
                } 
                bak[i] = pwm_value[i];     
            }   
        }
        cmd_str = cmd_str+"}";
        if(cmd_str.length() > 2) {
            Serial.println(cmd_str);
        }
        return;
                 
//        if(abs(bak[0]-pwm_value[0])>10 || abs(bak[1]-pwm_value[1])>10 || abs(bak[2]-pwm_value[2])>10 || abs(bak[3]-pwm_value[3])>10 || abs(bak[4]-pwm_value[4])>10) {
//            sprintf(cmd_return, "{#000P1500T1000!#001P%04dT0100!#002P%04dT0100!#003P%04dT0100!#004P%04dT0100!#005P%04dT0100!#006P1500T0000!#007P1500T0000!}", 
//                    3000-pwm_value[0], pwm_value[1], pwm_value[2], 3000-pwm_value[3], 3000-pwm_value[4]); 
//                    Serial.println(cmd_return);
//                    bak[0] = pwm_value[0];
//                    bak[1] = pwm_value[1];
//                    bak[2] = pwm_value[2];
//                    bak[3] = pwm_value[3];
//                    bak[4] = pwm_value[4];
//        }

      }
        
    }
    return; 
}
