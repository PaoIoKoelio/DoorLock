#include <Servo.h>
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME "locker"
#define BLYNK_AUTH_TOKEN ""

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#include <ESP_Mail_Client.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL ""
#define AUTHOR_PASSWORD ""

/* Recipient's email*/
#define RECIPIENT_EMAIL ""



#define DOOR_SENSOR_PIN  19
int doorState;
const char* ssid ="";
const char* pass = "";

static const int servoPin = 13;

Servo servo1;

void closeDoor(){
  servo1.write(30);
}

void openDoor(){
  servo1.write(150);
}

SMTP_Message message;

SMTPSession smtp;

Session_Config config;



// Callback function to get the Email sending status
void smtpCallback(SMTP_Status status);

void setup() {
  Serial.begin(115200);                    
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  servo1.attach(servoPin);
  Serial.println("Trying to connect to wifi");
  WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");


  MailClient.networkReconnect(true);

  // Enable the debug via Serial port
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);


  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  message.sender.name = F("ESP");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("INTRUDER ALERT");
  message.addRecipient(F("Vlado"), RECIPIENT_EMAIL);
    
  String textMsg = "Someone entered your home";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;


  /* Connect to the server */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }


  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

}

BLYNK_WRITE(V0)
{
  int pinValue = param.asInt(); // Get the value from the button (0 or 1)
  if(pinValue==1){
    openDoor();
  }
  else{
    closeDoor();
  }
}

size_t timer=0;
void loop() {
  Blynk.run();
  
  doorState = digitalRead(DOOR_SENSOR_PIN); // read state
  Blynk.virtualWrite(V1, doorState);
  if(timer>0){
    timer--;
  }
  Serial.println(timer);
  if (doorState == HIGH&&timer==0) {
    if (!smtp.connect(&config)){
      ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
      return;
    }
    if (MailClient.sendMail(&smtp, &message)) {
      Serial.println("Email sent successfully");
    } else {
      Serial.println("Failed to send email");
      Serial.println("Error: " + smtp.errorReason());
    }
  
  timer=100;
  }
  if (doorState == HIGH) {
    Serial.println("The door is open");
  }
   else {
    Serial.println("The door is closed");
  }
  delay(1000);
}


void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      SMTP_Result result = smtp.sendingResult.getItem(i);
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    smtp.sendingResult.clear();
  }
}



