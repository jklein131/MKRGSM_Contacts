#include <Modem.h>
#include <MKRGSM.h>
//GSM INFO
GSM gsmAccess;        // GSM access: include a 'true' parameter for debug enabled
boolean gsmconnected; // connection state

//SIM INFO 
unsigned long baud = 115200; 
const int timeout = 5000; // in milli seconds
const int sizer = 20; //i just made up this number
const int maxcontacts = 250;
char myNumber[20];
char myName[20];
int myId; 
int lastId = 0;
char contactbuf[sizer*2*maxcontacts];
String doAT(const char *  s) {
  String test;
  test.reserve(15);
  MODEM.send(s);
  MODEM.waitForResponse(500, &test);
  return test;
}
void addCONTACT(char* number, char * name){
  int contactid = 0; 
  contactid = _interface(printNULL,number,-1); //also refreshes between subsequent addCONTACT calls
  if(contactid == 0) {
    //didnt find number in phonebook
    updateCONTACT(lastId,number,name);

  }
  return;
}
void removeCONTACT(const char * phonenumber){
  //write contact at ID position
  int delId = 0;
  delId = _interface(printNULL,phonenumber,-1);
  //Serial.print(delId);
  if(delId == 0) {
    //no number matches do nothing 
    return;
  }
  char ait[5] = "";
  itoa(delId,ait,10);
  char s[12] = "AT+CPBW=";
  strcat(s,ait);
  doAT(s);
}
int exportCONTACT(int id, char * num, char * name, int len1, int len2) {
  int exportid;
  exportid = _interface(printNULL,"all",id);
  if(strlen(myName)-1 > len1) return -1;
  if(strlen(myNumber)-1 > len2) return -2;
  strcpy(name,myName);
  strcpy(num, myNumber );
}
char * getNAME(const char * phonenumber){
  //sets global contact info. 
  _interface(printNULL,phonenumber,-1);
  if(strcmp("",myNumber) ==0) {
    return NULL;
  }
  return (char *)myName; 
}
void updateCONTACT(int id, char* number, char * name){
  char cmd1[sizer] = "AT+CPBW=";
  char cmd2[5] = "";
  itoa(id,cmd2,10);
  strcat(cmd1,cmd2);
  strcat(cmd1,", \"");
  strcat(cmd1,number);
  strcat(cmd1,"\",145,\"");
  strcat(cmd1,name);
  strcat(cmd1,"\"");
  doAT(cmd1);
}
void printCONTACTS(){
  _interface(printSerial,"all",-1); //since its checking against a number, a character string will block aka "all"
}
void respondCONTACTS(void (*f)(const char * str)){
  //used for custom output handler (LCD, SMS, I2C etc.)
  _interface(f,"all",-1);
}
void printNULL(const char * str) {
  //does nothing
}
void printSerial(const char * str) {
  Serial.println(str);
}
int _interface(void (*f)(const char * str),const char * numberbreak,int idbreak){
  const int messagesize = 250;
  char names[messagesize] ="";
  String cmd; 
  char * pch;
  contactbuf[0] = '\0';
  myNumber[0] = '\0';
  myName[0] = '\0';
  names[0] = '\0';
  doAT("AT+CPBS=\"SM\"");  
  cmd = doAT("AT+CPBF=\"\"");
  strcpy(contactbuf,cmd.c_str());
  pch = strtok (contactbuf," ,\n\r\"");
  int colpos = 0; 
  while (pch != NULL)
  {
    if(colpos == 1){
      if(myId > lastId +1){
        //haha I know its a waste but helps me visualize
        lastId = lastId;
      } else {
        lastId = myId;
      }
      myId = atoi(pch);
      if(!((myId - lastId) > 1)){
        lastId = myId;
      }
      colpos++;
    }
    else if(colpos == 2){
      strcpy(myNumber,pch);
      colpos++;
    }
    else if(colpos == 3){
      //parse type here 
      colpos++;
    }
    else if(colpos == 4){
      strcpy(myName,pch);
      colpos++;
      strcat(names,"Contact ");
      char x[4];
      x[0] = '\0';
      itoa(myId,x,10);
      strcat(names,x);
      strcat(names," ");
      strcat(names,myName);
      strcat(names," ( "); //space after "(" to format number on android. 
      strcat(names,myNumber);
      strcat(names,")\n");
      if(strcmp(numberbreak,myNumber) ==0){
        return myId;
      }
      if(idbreak == myId) {
        return myId;
      }
    }
    if(strcmp(pch,"+CPBF:") == 0) {
      colpos = 1;
    }
    if((strlen(names) + 50) > messagesize){
      f(names);
      names[0] = '\0';
    }
    
    pch = strtok (NULL," ,\n\r\"");
  }
  if(strlen(names) > 0 ) {
    f(names);
    names[0] = '\0';
  }
  contactbuf[0] = '\0';
  myNumber[0] = '\0';
  myName[0] = '\0';
  if(myId == lastId){
        lastId = myId +1;
    } else {
      lastId+=1;
    }
  myId = 0;
  return myId;
}
void startGSM() {
  // Start GSM connection
  while (!gsmconnected) {
    Serial.print("CONNECTING TO GSM...");
    if (gsmAccess.begin() == GSM_READY) {
      gsmconnected = true;
      Serial.println("OK");
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }
}
void setup() {
  delay(5); //give hardware time to settle 
    
  #if (ARDUINO > 101)
  	do
  	{
  	// wait on serial port to be ready but timeout after 5 seconds
  	// this is for sytems that use virtual serial over USB.
  		if(millis() > 5000) // millis starts at 0 after reset
  			break;
  		delay(10); // easy way to allow some cores to call yield()
  	} while(!Serial);
  #endif
  
  startGSM(); //sets up the MODEM class for us... so that's nice
  
  //export the current contact 1 so we can use that slot. 
  char exportnum[20] = "";
  char exportname[100] = "";

  exportCONTACT(1,exportnum,exportname,20,100);
  
  Serial.println("current contacts");
  printCONTACTS();
  Serial.println("change contact 1");
  
  updateCONTACT(1,"+12345678910","test");
  printCONTACTS();
  
  Serial.println("delete contact 1");
  removeCONTACT("+12345678910");
  printCONTACTS();
  
  Serial.println("add new");
  addCONTACT("+66666666666","Devil");
  addCONTACT("+69696969696","Meme");
  printCONTACTS();
  removeCONTACT("+66666666666");
  removeCONTACT("+69696969696");
  
  Serial.println("restore phonebook #1");
  updateCONTACT(1,exportnum,exportname);
  printCONTACTS();
}
void loop(){
  
}
