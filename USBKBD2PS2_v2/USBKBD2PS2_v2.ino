
//
// USBKBD2PS2_v2 USBキーボード PS/2キーボード変換 for IchigoJam v2
// 作成者 たま吉さん
// 作成日 2016/11/11 , 最終修正日 2016/11/18
// 修正 2016/11/18 Bluetooth HIDとUSB HIDのスケッチの統合,キーリピート機能対応
//
// このスケッチの利用には以下のハードウェア(シールド)が必要です.
//  ・USB Host Shield
// 
// このスケッチのコンパイルには以下のライブラリが必要です.
//  ・PS/2デバイス ps2dev(ps2dev.zip) - an interface library for ps2 host 
//    PS2 mouse interface for Arduino(http://playground.arduino.cc/ComponentLib/Ps2mouse)
//  ・USB_Host_Shield_2.0 (https://github.com/felis/USB_Host_Shield_2.0)
//  ・MsTimer2 (http://playground.arduino.cc/Main/MsTimer2)
//

//******* 用途により、下記の定義を設定して下さい ***************************
#define MYDEBUG      0  // 0:デバッグ情報出力なし 1:デバッグ情報出力あり 
#define KB_CLK      A4  // PS/2 CLK  IchigoJamのKBD1に接続
#define KB_DATA     A5  // PS/2 DATA IchigoJamのKBD2に接続
//**************************************************************************

#include <ps2dev.h>
#include <BTHID.h>
#include <hidboot.h>
#include <usbhub.h>
#include <MsTimer2.h>

#define LOBYTE(x) ((char*)(&(x)))[0]
#define HIBYTE(x) ((char*)(&(x)))[1]

// キーリピートの定義
#define REPEATTIME      5   // キーを押し続けて、REP_INTERVALxREPEATTIMEmsec後にリピート開始
#define EMPTY           0   // リピート管理テーブルが空状態
#define MAXKEYENTRY     6   // リピート管理テーブルサイズ
#define REP_INTERVAL    100 // リピート間隔 150msec

#define MS_SIKIICHI     10

uint8_t keyentry[MAXKEYENTRY];    // リピート管理テーブル
uint8_t repeatWait[MAXKEYENTRY];  // リピート開始待ち管理テーブル
uint8_t enabled =0;               // PS/2 ホスト送信可能状態
PS2dev keyboard(KB_CLK, KB_DATA); // PS/2デバイス

//
// HIDキーボード レポートパーサークラスの定義
//
class KbdRptParser : public KeyboardReportParser {
  protected:
    virtual uint8_t HandleLockingKeys(USBHID *hid, uint8_t key);
    virtual void OnControlKeysChanged(uint8_t before, uint8_t after);
    virtual void OnKeyDown(uint8_t mod, uint8_t key);
    virtual void OnKeyUp(uint8_t mod, uint8_t key);
    virtual void OnKeyPressed(uint8_t key) {};
};

// HIDマウス レポートパーサークラスの定義
/*
class MouseRptParser : public MouseReportParser {
  protected:
    virtual void OnMouseMove(MOUSEINFO *mi) {};
    virtual void OnLeftButtonUp(MOUSEINFO *mi){};
    virtual void OnLeftButtonDown(MOUSEINFO *mi){};
    virtual void OnRightButtonUp(MOUSEINFO *mi) {};
    virtual void OnRightButtonDown(MOUSEINFO *mi){};
    virtual void OnMiddleButtonUp(MOUSEINFO *mi){};
    virtual void OnMiddleButtonDown(MOUSEINFO *mi){};
};
*/

class MouseRptParser : public MouseReportParser {
  protected:
    void OnMouseMove(MOUSEINFO *mi);
    void OnLeftButtonUp(MOUSEINFO *mi);
    void OnLeftButtonDown(MOUSEINFO *mi);
    void OnRightButtonUp(MOUSEINFO *mi);
    void OnRightButtonDown(MOUSEINFO *mi);
    void OnMiddleButtonUp(MOUSEINFO *mi);
    void OnMiddleButtonDown(MOUSEINFO *mi);
};

void MouseRptParser::OnMouseMove(MOUSEINFO *mi){
/*  
  Serial.print("dx=");
  Serial.print(mi->dX, DEC);
  Serial.print(" dy=");
  Serial.println(mi->dY, DEC);
*/
  if (mi->dX > MS_SIKIICHI) {
    keyboard.write(0xE0);
    keyboard.write(0x74);
    delay(50); 
    keyboard.write(0xE0);
    keyboard.write(0xF0);
    keyboard.write(0x74);
  } else if (mi->dX < -MS_SIKIICHI) {
    keyboard.write(0xE0);
    keyboard.write(0x6B);    
    delay(50); 
    keyboard.write(0xE0);
    keyboard.write(0xF0);
    keyboard.write(0x6B);    
  }
  if (mi->dY > MS_SIKIICHI) {
    keyboard.write(0xE0);
    keyboard.write(0x72);        
    delay(50); 
    keyboard.write(0xE0);
    keyboard.write(0xF0);
    keyboard.write(0x72);        
  } else if (mi->dY < -MS_SIKIICHI) {
    keyboard.write(0xE0);
    keyboard.write(0x75);        
    delay(50); 
    keyboard.write(0xE0);
    keyboard.write(0xF0);
    keyboard.write(0x75);        
  }
}

void MouseRptParser::OnLeftButtonUp  (MOUSEINFO *mi){
  Serial.println("L Butt Up");
}

void MouseRptParser::OnLeftButtonDown (MOUSEINFO *mi){
  Serial.println("L Butt Dn");
}

void MouseRptParser::OnRightButtonUp  (MOUSEINFO *mi){
  Serial.println("R Butt Up");
}

void MouseRptParser::OnRightButtonDown  (MOUSEINFO *mi){
  Serial.println("R Butt Dn");
}

void MouseRptParser::OnMiddleButtonUp (MOUSEINFO *mi){
  Serial.println("M Butt Up");
}

void MouseRptParser::OnMiddleButtonDown (MOUSEINFO *mi){
  Serial.println("M Butt Dn");
}

USB      Usb;
USBHub  Hub1(&Usb);
USBHub  Hub2(&Usb);
USBHub  Hub3(&Usb);
USBHub  Hub4(&Usb);

BTD     Btd(&Usb);
BTHID   bthid(&Btd, PAIR, "0000");
BTHID   hid(&Btd);

KbdRptParser keyboardPrs;
MouseRptParser mousePrs;

HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE>    HidMouse(&Usb);

uint8_t classType = 0;      
uint8_t subClassType = 0;
uint32_t next_time;

// HID Usage ID (0x04 - 0x67) => PS/2 スキャンコード 変換テーブル
const uint8_t keytable1[] PROGMEM = {
  0x1C,0, 0x32,0, 0x21,0, 0x23,0, 0x24,0, 0x2B,0, 0x34,0, 0x33,0, // 0x04 - 0x0B
  0x43,0, 0x3B,0, 0x42,0, 0x4B,0, 0x3A,0, 0x31,0, 0x44,0, 0x4D,0, // 0x0C - 0x13
  0x15,0, 0x2D,0, 0x1B,0, 0x2C,0, 0x3C,0, 0x2A,0, 0x1D,0, 0x22,0, // 0x14 - 0x1B
  0x35,0, 0x1A,0, 0x16,0, 0x1E,0, 0x26,0, 0x25,0, 0x2E,0, 0x36,0, // 0x1C - 0x23
  0x3D,0, 0x3E,0, 0x46,0, 0x45,0, 0x5A,0, 0x76,0, 0x66,0, 0x0D,0, // 0x24 - 0x2B
  0x29,0, 0x4E,0, 0x55,0, 0x54,0, 0x5B,0, 0x5D,0, 0x5D,0, 0x4C,0, // 0x2C - 0x33
  0x52,0, 0x0E,0, 0x41,0, 0x49,0, 0x4A,0, 0x58,0, 0x05,0, 0x06,0, // 0x34 - 0x3B
  0x04,0, 0x0C,0, 0x03,0, 0x0B,0, 0x83,0, 0x0A,0, 0x01,0, 0x09,0, // 0x3C - 0x43
  0x78,0, 0x07,0, 0x00,2, 0x7E,0, 0x00,3, 0x70,1 ,0x6C,1, 0x7D,1, // 0x44 - 0x4B
  0x71,1, 0x69,1, 0x7A,1, 0x74,1, 0x6B,1, 0x72,1 ,0x75,1, 0x77,0, // 0x4C - 0x53
  0x4A,1, 0x7C,0, 0x7B,0, 0x79,0, 0x5A,1, 0x69,0, 0x72,0, 0x7A,0, // 0x54 - 0x5B
  0x6B,0, 0x73,0, 0x74,0, 0x6C,0, 0x75,0, 0x7D,0, 0x70,0, 0x71,0, // 0x5C - 0x63
  0x61,0, 0x2F,1, 0x37,1, 0x0F,0,                                 // 0x64 - 0x67
};

// HID Usage ID (0x87 - 0x94) => PS/2 スキャンコード 変換テーブル
const uint8_t keytable2[] PROGMEM = {
  0x51,0, 0x13,0, 0x6A,0, 0x64,0, 0x67,0, 0x27,0, 0x00,0, 0x00,0, // 0x87 - 0x8E
  0x00,0, 0xF2,0, 0xF1,0, 0x63,0, 0x62,0, 0x5F,0,                 // 0x8F - 0x94
};

// PS/2 ホストにack送信
void ack() {
  while(keyboard.write(0xFA));
}

// PS/2 ホストから送信されるコマンドの処理
int keyboardcommand(int command) {
  unsigned char val;
  switch (command) {
  case 0xFF:  ack();// Reset: キーボードリセットコマンド。正しく受け取った場合ACKを返す。
    //while(keyboard.write(0xAA)!=0);
    break;
  case 0xFE: // 再送要求
    ack();
    break;
  case 0xF6: // 起動時の状態へ戻す
    //enter stream mode
    ack();
    break;
  case 0xF5: //起動時の状態へ戻し、キースキャンを停止する
    //FM
    enabled = 0;
    ack();
    break;
  case 0xF4: //キースキャンを開始する
    //FM
    enabled = 1;
    ack();
    break;
  case 0xF3: //set typematic rate/delay : 
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  case 0xF2: //get device id : 
    ack();
    keyboard.write(0xAB);
    keyboard.write(0x83);
    break;
  case 0xF0: //set scan code set
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  case 0xEE: //echo :キーボードが接続されている場合、キーボードはパソコンへ応答（ECHO Responce）を返す。
    //ack();
    keyboard.write(0xEE);
    break;
  case 0xED: //set/reset LEDs :キーボードのLEDの点灯/消灯要求。これに続くオプションバイトでLEDを指定する。 
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  }
}

// リピート管理テーブルのクリア
void claerKeyEntry() {
 for (uint8_t i=0; i <MAXKEYENTRY; i++)
    keyentry[i] = EMPTY;
}

// リピート管理テーブルにキーを追加
void addKey(uint8_t key) {
 for (uint8_t i=0; i <MAXKEYENTRY; i++) {
  if (keyentry[i] == EMPTY) {
    keyentry[i] = key;  
    repeatWait[i] = REPEATTIME;
    break;
  }
 }
}

// リピート管理テーブルからキーを削除
void delKey(uint8_t key) {
 for (uint8_t i=0; i <MAXKEYENTRY; i++) {
  if (keyentry[i] == key) {
    keyentry[i] = EMPTY;
    break;
  }
 }  
}

//
// PS/2 makeコード送信
// 引数 key(IN) HID Usage ID
//
uint8_t sendKeyMake(uint8_t key) {
  // HID Usage ID から PS/2 スキャンコード に変換
  uint8_t code = 0;
  uint8_t pre = 0xff;

  if (key >= 0x04 && key <= 0x67) {
    code = pgm_read_byte(keytable1 + (key - 0x04)*2);
    pre = pgm_read_byte(keytable1 + (key - 0x04)*2 + 1);
  } else if (key >= 0x87 && key <= 0x94) {
    code = pgm_read_byte(keytable2 + (key - 0x87)*2);    
    pre = pgm_read_byte(keytable2 + (key - 0x87)*2 + 1);    
  }

  // PS/2キーの発行
  if (pre == 0) {
    keyboard.write(code);
    return 1;
  } else if (pre == 1) {
    keyboard.write(0xE0);
    keyboard.write(code);    
  } else if (pre == 2) { // PrintScreenキー
    keyboard.write(0xE0);
    keyboard.write(0x12);
    keyboard.write(0xE0);
    keyboard.write(0x7C);
  } else if (pre == 3) { // Pauseキー
    keyboard.write(0xE1);
    keyboard.write(0x14);
    keyboard.write(0x77);
    keyboard.write(0xE1);
    keyboard.write(0xF0);    
    keyboard.write(0x14);
    keyboard.write(0xF0);    
    keyboard.write(0x77);
  }
  return code;
}

//
// PS/2 breakコード送信
// 引数 key(IN) HID Usage ID
//
uint8_t sendKeyBreak(uint8_t key) {
  // HID Usage ID から PS/2 スキャンコード に変換
  uint8_t code = 0;
  uint8_t pre = 0xff;
 
  if (key >= 0x04 && key <= 0x67) {
    code = pgm_read_byte(keytable1 + (key - 0x04)*2);
    pre = pgm_read_byte(keytable1 + (key - 0x04)*2 + 1);
  } else if (key >= 0x87 && key <= 0x94) {
    code = pgm_read_byte(keytable2 + (key - 0x87)*2);    
    pre = pgm_read_byte(keytable2 + (key - 0x87)*2 + 1);    
  }

  // PS/2キーの発行
  if (pre == 0) {
    keyboard.write(0xF0);
    keyboard.write(code);    
  } else if (pre == 1) {
    keyboard.write(0xE0);
    keyboard.write(0xF0);
    keyboard.write(code);    
  } else if (pre == 2) {
    keyboard.write(0xE0);
    keyboard.write(0xF0);
    keyboard.write(0x7C);
    keyboard.write(0xE0);
    keyboard.write(0xF0);
    keyboard.write(0x12);
  }
  return code;
}

// リピート処理(タイマー割り込み処理から呼ばれる)
void sendRepeat() {
  // HID Usage ID から PS/2 スキャンコード に変換
  uint8_t code = 0;
  uint8_t pre, key;
  
  for (uint8_t i=0; i < MAXKEYENTRY; i++) {
    if (keyentry[i] != EMPTY) {
      key = keyentry[i]; 
      if (repeatWait[i] == 0) {
        sendKeyMake(key);
    } else {
        repeatWait[i]--;          
      }
    }
  }
}

//
// ロックキー（NumLock/CAPSLock/ScrollLock)ハンドラ
//

uint8_t KbdRptParser::HandleLockingKeys(USBHID *hid, uint8_t key) {
  if (classType == USB_CLASS_WIRELESS_CTRL) {
    uint8_t old_keys = kbdLockingKeys.bLeds;  
    switch (key) {
      case UHS_HID_BOOT_KEY_NUM_LOCK:
        kbdLockingKeys.kbdLeds.bmNumLock = ~kbdLockingKeys.kbdLeds.bmNumLock;
        break;
      case UHS_HID_BOOT_KEY_CAPS_LOCK:
        kbdLockingKeys.kbdLeds.bmCapsLock = ~kbdLockingKeys.kbdLeds.bmCapsLock;
        break;
      case UHS_HID_BOOT_KEY_SCROLL_LOCK:
        kbdLockingKeys.kbdLeds.bmScrollLock = ~kbdLockingKeys.kbdLeds.bmScrollLock;
        break;
    }
    if (old_keys != kbdLockingKeys.bLeds && hid) {
      BTHID *pBTHID = reinterpret_cast<BTHID *> (hid); // A cast the other way around is done in BTHID.cpp
      pBTHID->setLeds(kbdLockingKeys.bLeds); // Update the LEDs on the keyboard
    }
  } else {
    return KeyboardReportParser::HandleLockingKeys(hid, key);   
  }
  return 0;
}


//
// キー押しハンドラ
// 引数
//  mod : コントロールキー状態
//  key : HID Usage ID 
//
void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key) {
  MsTimer2::stop();
#if MYDEBUG==1
  Serial.print(F("DN ["));  Serial.print(F("mod="));  Serial.print(mod,HEX);
  Serial.print(F(" key="));  Serial.print(key,HEX);  Serial.println(F("]"));
#endif
  if (sendKeyMake(key))
    addKey(key);
  MsTimer2::start();
}

//
// コントロールキー変更ハンドラ
// SHIFT、CTRL、ALT、GUI(Win)キーの処理を行う
// 引数 before : 変化前のコード USB Keyboard Reportの1バイト目
//      after  : 変化後のコード USB Keyboard Reportの1バイト目
//
void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {
  MODIFIERKEYS beforeMod;
  *((uint8_t*)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t*)&afterMod) = after;

  // 左Ctrlキー
  if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
    if (afterMod.bmLeftCtrl) {
      keyboard.write(0x14);  // 左Ctrlキーを押した
    } else {
      keyboard.write(0xF0);  // 左Ctrltキーを離した       
      keyboard.write(0x14);
    } 
  }

  // 左Shiftキー
  if (beforeMod.bmLeftShift != afterMod.bmLeftShift) {
    if (afterMod.bmLeftShift) {
      keyboard.write(0x12);  // 左Shiftキーを押した
    } else {
      keyboard.write(0xF0);  // 左Shiftキーを離した       
      keyboard.write(0x12);  // 
    }
  }

  // 左Altキー
  if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt) {
    if (afterMod.bmLeftAlt) {
      keyboard.write(0x11);  // 左Altキーを押した
    } else {
      keyboard.write(0xF0);  // 左Altキーを離した       
      keyboard.write(0x11);  // 
    }
  }

  // 左GUIキー(Winキー)
  if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI) {
    if (afterMod.bmLeftGUI) {
      keyboard.write(0xE0);  // 左GUIキーを押した
      keyboard.write(0x1F);
    } else {
      keyboard.write(0xE0);  // 左GUIキーを離した       
      keyboard.write(0xF0); 
      keyboard.write(0x1F); 
    }
  }

  // 右Ctrlキー
  if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
    if (afterMod.bmRightCtrl) {
      keyboard.write(0xE0);  // 右Ctrlキーを押した
      keyboard.write(0x14);  
    } else {
      keyboard.write(0xE0);  // 右Ctrlキーを離した 
      keyboard.write(0xF0);       
      keyboard.write(0x14);
    }
  }

  // 右Shiftキー
  if (beforeMod.bmRightShift != afterMod.bmRightShift) {
    if (afterMod.bmRightShift) {
      keyboard.write(0x59);  // 右Shiftキーを押した
    } else {
      keyboard.write(0xF0);  // 右Shiftキーを離した       
      keyboard.write(0x59); 
    }
  }

  // 右Altキー
  if (beforeMod.bmRightAlt != afterMod.bmRightAlt) {
    if (afterMod.bmRightAlt) {
      keyboard.write(0xE0);  // 右Altキーを押した
      keyboard.write(0x11); 
    } else {
      keyboard.write(0xE0);  // 右Altキーを離した       
      keyboard.write(0xF0);
      keyboard.write(0x11); 
    };
  }

  // 右GUIキー
  if (beforeMod.bmRightGUI != afterMod.bmRightGUI) {
    if (afterMod.bmRightGUI) {
      keyboard.write(0xE0);  // 右GUIキーを押した
      keyboard.write(0x27);
    } else {
      keyboard.write(0xE0);  // 右GUIキーを離した       
      keyboard.write(0xF0); 
      keyboard.write(0x27); 
    }
  }
}

//
// キー離し ハンドラ
// 引数
//  mod : コントロールキー状態
//  key : HID Usage ID 
//
void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key) {
  MsTimer2::stop();
#if MYDEBUG==1
  Serial.print(F("UP ["));  Serial.print(F("mod="));  Serial.print(mod,HEX);
  Serial.print(F(" key="));  Serial.print(key,HEX);  Serial.println(F("]"));
#endif
  if (sendKeyBreak(key)) // HID Usage ID から PS/2 スキャンコード に変換  
    delKey(key);  
  MsTimer2::start();
}

//
// インターフェースクラスの取得
//
uint8_t getIntClass(byte& intclass, byte& intSubClass ) {
  uint8_t buf[ 256 ];
  uint8_t* buf_ptr = buf;
  byte rcode;
  byte descr_length;
  byte descr_type;
  unsigned int total_length;

  uint8_t flgFound = 0;
  
  //デスクプリタトータルサイズの取得
  rcode = Usb.getConfDescr( 1, 0, 4, 0, buf );
  LOBYTE( total_length ) = buf[ 2 ]; HIBYTE( total_length ) = buf[ 3 ];
  if ( total_length > 256 ) {
    total_length = 256;
  }
  
  rcode = Usb.getConfDescr( 1, 0, total_length, 0, buf ); 
  while ( buf_ptr < buf + total_length ) { 
    descr_length = *( buf_ptr );
    descr_type = *( buf_ptr + 1 );

    if (descr_type == USB_DESCRIPTOR_INTERFACE) {
      // 最初のインタフェースの取得
      USB_INTERFACE_DESCRIPTOR* intf_ptr = ( USB_INTERFACE_DESCRIPTOR* )buf_ptr;  
      intclass = intf_ptr->bInterfaceClass;
      intSubClass = intf_ptr->bInterfaceSubClass;
      flgFound = 1;
      break;
    }
    buf_ptr = ( buf_ptr + descr_length );    //advance buffer pointer
  }
  return ( flgFound );
}

void setup() {
  Serial.begin( 115200 );
  while(keyboard.write(0xAA)!=0);
  if (Usb.Init() == -1) {
    Serial.println(F("OSC did not start."));
    while (1); // Halt    
  }
  
  next_time = millis() + 5000;

  bthid.SetReportParser(KEYBOARD_PARSER_ID, &keyboardPrs);
  bthid.SetReportParser(MOUSE_PARSER_ID, &mousePrs);
  bthid.setProtocolMode(USB_HID_BOOT_PROTOCOL); // Boot Protocol Mode
  bthid.setProtocolMode(HID_RPT_PROTOCOL); // Report Protocol Mode
  //HidKeyboard.SetReportParser(0, &Prs);
  HidKeyboard.SetReportParser(0, &keyboardPrs);
  HidMouse.SetReportParser(0, &mousePrs);

  claerKeyEntry();
  MsTimer2::set(REP_INTERVAL, sendRepeat); 
  MsTimer2::start();
    
  Serial.println(F("Start."));
}

void loop() {
  unsigned char c;  // ホストからの送信データ
  if( (digitalRead(KB_CLK)==LOW) || (digitalRead(KB_DATA) == LOW)) {
    while(keyboard.read(&c)) ;
    keyboardcommand(c);
  }  
  Usb.Task();
  if ( (classType == 0) && (Usb.getUsbTaskState() == USB_STATE_RUNNING) )  {  
    // デバイスクラス情報の取得
    getIntClass(classType, subClassType) ;
#if MYDEBUG == 1  
    Serial.print(F("class="));  Serial.println(classType, HEX);
    Serial.print(F("subclass="));  Serial.println(subClassType, HEX);
#endif
  }
  
#if MYDEBUG == 1  
  static uint8_t prevSts = 0xFF;
  if (Usb.getUsbTaskState() != prevSts) {
    prevSts = Usb.getUsbTaskState();
    Serial.print(F("sts="));
    Serial.println(prevSts, HEX);
  }
  if (Serial.available()) {
    Serial.read();
    for (uint8_t i=-0; i < MAXKEYENTRY; i++) {
      Serial.print(F("keyentry["));
      Serial.print(i,DEC);
      Serial.print(F("]="));
      Serial.println(keyentry[i],HEX);
    }
  }
#endif
}

