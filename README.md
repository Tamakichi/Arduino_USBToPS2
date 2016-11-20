# Arduino_USBToPS2
Arduino USB=> PS/2 変換モジュール  

![トップ画像](./img/top.jpg)  

## 概要  
Arduino pro mini 3.3V 8Mhz版＋USBホストシールドを使ったUSB=>PS/2変換を行うモジュールです。  
本モジュールを使うことで、IchigoJamにてUSB HIDキーボード、bluetooth HIDキーボード(Bluetoothドングル利用時）、  
の利用が可能となります。  

### システム構成図  
![システム構成](./img/system.png)   

### 接続例1  
IchigoJamのVCC、GND、KBD1、KBD2端子に接続  
![システム構成](./img/ichigojam.jpg)   

### 接続例2  
IchigoJamのUSBコネクタに接続    
![システム構成](./img/ichigojam2.jpg)   

### 動作確認したキーボード  
![キーボード](./img/keyboard.jpg)   

## 関連記事  
・猫にコ・ン・バ・ン・ワ  - IchigoJam用 USBキーボード変換 Bluetooth・USBキーボードの両対応できました2016/11/19    
  http://nuneno.cocolog-nifty.com/blog/2016/11/ichigojam-usb-b.html  
・猫にコ・ン・バ・ン・ワ  - IchigoJamでLogicool製ワイヤレスキーボードを利用する(USB・PS/2変換) その2  
  http://nuneno.cocolog-nifty.com/blog/2016/11/ichigojamlogi-1.html  
・猫にコ・ン・バ・ン・ワ  - IchigoJamでLogicool製ワイヤレスキーボードを利用する(USB・PS/2変換)  
  http://nuneno.cocolog-nifty.com/blog/2016/11/ichigojamlogico.html  
・猫にコ・ン・バ・ン・ワ  - IchigoJamでポケモンキーボードを使ってみる(Bluetooth・PS/2変換)  
  http://nuneno.cocolog-nifty.com/blog/2016/11/ichigojam-f720.html  
・猫にコ・ン・バ・ン・ワ  - Arduino Pro mini Pro Mini用USB HOSTシールドの調査  
  http://nuneno.cocolog-nifty.com/blog/2016/11/arduino-pro-m-1.html  
