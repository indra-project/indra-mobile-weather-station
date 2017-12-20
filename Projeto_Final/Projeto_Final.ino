#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <Thread.h>
#include <ThreadController.h>
#include <TinyGPS++.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS 2 // Porta do pino de sinal do DS18B20 TEMPERATURA


//Ethernet Shield
//IPAddress ip(192,168,0,51);
//IPAddress gateway(192,168,0,1);
//IPAddress subnet(255,255,255,0);
//EthernetServer server(80);
//byte mac[] = { 0x90, 0x02, 0xBA, 0xEF, 0xCA, 0x33 };
//Sensores
OneWire oneWire(ONE_WIRE_BUS); // Define uma instancia do oneWire para comunicacao com o sensor
DallasTemperature sensors(&oneWire);
DeviceAddress sensor1;
//CHUVA
int pino_a = A3; //Pino ligado ao A3 do sensor
//MONOX
const int AOUTpin = A2;//the AOUT pin of the CO sensor goes into analog pin A0 of the arduino
//UV
int pino_sensor = A0; //Output from the sensor
//Objeto TinyGPS++
TinyGPSPlus gps;
//criar um objeto File para manipular um arquivo
File myFile;
String inputTemp = "", inputRad = "", inputMon = "", inputLatLang = "", inpuChuva = "", inputDados = "";
String confgEnable = "", configAtualizacao = "", configAux="";
int enable, intervalTemp, intervalMonoxido, intervalGps, intervalRadiacao, intervalChuva, intervalDados;
int sizeJsonGps = 0, sizeJsonTemp = 0, sizeJsonMon = 0, sizeJsonRad = 0, sizeJsonDados = 0, sizeJsonChuva = 0, connectIsValid = 0;
String stringJson = "";
double lati, lang;
String horaGPS = "";
//Criar a threads para leitura e escrita dos dados
Thread threadConnectGPS = Thread();
Thread threadWriteTemp = Thread();
Thread threadWriteRad = Thread();
Thread threadWriteMon = Thread();
Thread threadWriteChuva = Thread();
Thread threadWriteDados = Thread();
//Criar a threads para leitura dos dados de seus respectivos arquivos
Thread threadReadGPS = Thread();
Thread threadReadTemp = Thread();
Thread threadReadRad = Thread();
Thread threadReadMon = Thread();
Thread threadReadChuva = Thread();
Thread threadReadDados = Thread();
//Criando um controlador das threads
ThreadController controller = ThreadController();
 
void setup() {
  Serial.begin(4800);
  Serial3.begin(9600);
  // As tentativas de estabelecer uma conexão Ethernet baseado DHCP
//  if (Ethernet.begin(mac) == 0) {
//    Serial.println("Conexão via DHCP não pôde ser estabelecida. Give 'endereço IP fixo");
//    Ethernet.begin(mac); // Se nenhum DHCP encontrado, utilizar o 192.168.1.177 IP estático
//  }else{
//    Serial.println("ok");
//  }
  
  while (!Serial);
  Serial.println("Inicializa o SD card...");
  if (!SD.begin(4)) {
    Serial.println("Nao inicializado...");
    return;
  }
  //Criando arquivos de configurações - config.txt e enable.txt
  //Criando os arquivos p/ a plataforma - 1 p/ habilitar e 0 p/ desabilitar
  //Os tempos para o arquivo config.txt
  writeSdCardConfig(createJsonEnable(1), "enable.txt", false);
  writeSdCardConfig(createJsonConfig(5000, 5000, 5000, 5000, 5000, 5000), "config.txt", false);
  //Lendo os arquivos de configuração
  confgEnable = readSdCardConfig("enable.txt");
  configAtualizacao = readSdCardConfig("config.txt");
  parseJsonConfig(confgEnable);
  parseJsonConfig(configAtualizacao);
  
  //inicio de thread para realizar conexão c/ o módulo GPS e escrever no arquivo gps.txt
  threadConnectGPS.enabled = true;
  threadConnectGPS.setInterval(intervalGps);
  threadConnectGPS.onRun(printAndWritePositionGPS);
  controller.add(&threadConnectGPS);  
  //inicio de thread para realizar escrita de temperatura no arquivo temp.txt
  threadWriteTemp.enabled = true;
  threadWriteTemp.setInterval(intervalTemp);
  threadWriteTemp.onRun(printAndWriteTemperature);
  controller.add(&threadWriteTemp);
  //inicio de thread para realizar escrita de radiacao no arquivo rad.txt
  threadWriteRad.enabled = true;
  threadWriteRad.setInterval(intervalRadiacao);
  threadWriteRad.onRun(printAndWriteRaiation);
  controller.add(&threadWriteRad);
  //inicio de thread para realizar escrita de Monóxido de Carbono no arquivo mon.txt
  threadWriteMon.enabled = true;
  threadWriteMon.setInterval(intervalMonoxido);
  threadWriteMon.onRun(printAndWriteMon);
  controller.add(&threadWriteMon);
  //inicio de thread para realizar escrita de chuva no arquivo chuva.txt
  threadWriteChuva.enabled = true;
  threadWriteChuva.setInterval(intervalChuva);
  threadWriteChuva.onRun(printAndWriteChuva);
  controller.add(&threadWriteChuva);
  //inicio de thread para realizar escrita de dados no arquivo dados.txt
  threadWriteDados.enabled = true;
  threadWriteDados.setInterval(intervalDados);
  threadWriteDados.onRun(printAndWriteDados);
  controller.add(&threadWriteDados);

  //inicio de thread para realizar a leitura arquivo gps.txt
  threadReadGPS.enabled = true;
  threadReadGPS.setInterval(intervalGps);
  threadReadGPS.onRun(printAndReadPositionGPS);
  controller.add(&threadReadGPS);
  //inicio de thread para realizar a leitura arquivo gps.txt
  threadReadTemp.enabled = true;
  threadReadTemp.setInterval(intervalTemp);
  threadReadTemp.onRun(printAndReadTemperature);
  controller.add(&threadReadTemp);
  //inicio de thread para realizar a leitura arquivo gps.txt
  threadReadRad.enabled = true;
  threadReadRad.setInterval(intervalRadiacao);
  threadReadRad.onRun(printAndReadRaiation);
  controller.add(&threadReadRad);
  //inicio de thread para realizar a leitura arquivo mon.txt
  threadReadMon.enabled = true;
  threadReadMon.setInterval(intervalMonoxido);
  threadReadMon.onRun(printAndReadMon);
  controller.add(&threadReadMon);
  //inicio de thread para realizar a leitura arquivo mon.txt
  threadReadChuva.enabled = true;
  threadReadChuva.setInterval(intervalChuva);
  threadReadChuva.onRun(printAndReadChuva);
  controller.add(&threadReadChuva);
  //inicio de thread para realizar a leitura arquivo mon.txt
  threadReadDados.enabled = true;
  threadReadDados.setInterval(intervalDados);
  threadReadDados.onRun(printAndReadDados);
  controller.add(&threadReadDados);

  //TEMPERAURA
  sensors.begin();
  Serial.println("Localizando sensores DS18B20...");
  Serial.print("Foram encontrados ");
  Serial.print(sensors.getDeviceCount(), DEC);   // Localiza e mostra enderecos dos sensores
  Serial.println(" sensores.");
  if (!sensors.getAddress(sensor1, 0)) 
     Serial.println("Sensores nao encontrados !"); 
  Serial.print("Endereco sensor: ");
  mostra_endereco_sensor(sensor1);  // Mostra o endereco do sensor encontrado no barramento
  Serial.println();
  Serial.println();
  //CHUVA
  pinMode(pino_a, INPUT);
  //GAS
  //pinMode(DOUTpin, INPUT);//sets the pin as an input to the arduino
  //UV
  pinMode(pino_sensor, INPUT);
  
  Serial.println("Beleza! Seguindo...");  
//  connectGPS();
}
//Get enderços sensores
void mostra_endereco_sensor(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // Adiciona zeros se necess?rio
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
//Atribui config a variaveis globais
void parseJsonConfig(String jsonConfig){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonConfig);
  String type = root[String("TYPE")];
  if(type == "ENABLE"){
    enable = root[String("status")];
  }else if(type == "CONFIG"){
    intervalRadiacao = root[String("radiacao")];
    intervalTemp = root[String("temperatura")];
    intervalMonoxido = root[String("monoxido")];
    intervalGps = root[String("gps")];
    intervalChuva =root[String("chuva")];
    intervalDados =root[String("dados")];
  }
  
}
//Lendo os aquivos de configurações
String readSdCardConfig(String nameFile){
  String json = "";
  myFile = SD.open(nameFile);
  if (myFile) {
    // Le todo o arquivo...
    while (myFile.available()) {
      char c = myFile.read();
      json = json + c;   
    }
    myFile.close();
    return json;
  }
  else {
    // Erro ao abrir o arquivo cujo nome vem por parâmetro nesta função
    Serial.print("Erro ao abrir o arquivo ");
    Serial.print(nameFile);
    Serial.println("...");
    return "";
  }
}

//Criando os arquivos que vão controlar o status do protótipo e
//sua taxa de atualização
void writeSdCardConfig(String json, String nameFile, boolean enable){
   myFile = SD.open(nameFile);
   if(myFile && enable == false){
    myFile.close();
   }
   else{
    deleteFileSdCard(nameFile);
    myFile = SD.open(nameFile, FILE_WRITE);
    //  Se o arquivo foi aberto com sucesso, escreve nele
    if (myFile) {
      Serial.println("Criando configs....");
      myFile.println(json);
      myFile.close(); 
    }
    else {
      // Erro ao abrir o arquivo cujo nome vem por parâmetro nesta função
      Serial.print("Erro ao abrir o arquivo ");
      Serial.print(nameFile);
      Serial.println("...");
    }
   }
}
//Criar arquivos e add linhas de dados se existente
void writeSdCard(String json, String nameFile){
   myFile = SD.open(nameFile, FILE_WRITE);
//  Se o arquivo foi aberto com sucesso, escreve nele
  if (myFile) {
    Serial.print("Escrevendo em ");Serial.print(nameFile);Serial.println("...");
    myFile.println(json);
    myFile.close(); 
  }
  else {
    // Erro ao abrir o arquivo cujo nome vem por parâmetro nesta função
    Serial.print("Erro ao abrir o arquivo ");
    Serial.print(nameFile);
    Serial.println("...");
  }
}
//Função que calcula a quantidade de dado em um determinado arquivo
int sizeFile(String nameFile){
  int sizeFile = 0;
   myFile = SD.open(nameFile);
   if (myFile) {
    // Le todo o arquivo...
    while (myFile.available()) {
      char c = myFile.read();    
      if (c == '\n'){
        sizeFile++;
      }
    }
    myFile.close();
  }
  else {
    // Erro ao abrir o arquivo cujo nome vem por parâmetro nesta função
      Serial.print("Erro ao abrir o arquivo ");
      Serial.print(nameFile);
      Serial.println("...");
    return -1;
  }
  return sizeFile;
}
//Abre um arquivo existente para realizar uma leitura
void readSdCard(String nameFile){
  // Agora vamos abri-lo para leitura
  int contador = 0;
  myFile = SD.open(nameFile);
  if (myFile) {
    // Le todo o arquivo...
    while (myFile.available()) {
      char c = myFile.read();  
      stringJson = stringJson + c;
    }
    Serial.print("###############DADOS GERAIS: ");Serial.println(stringJson);
    myFile.close();
  }
  else {
    // Erro ao abrir o arquivo cujo nome vem por parâmetro nesta função
    Serial.print("Erro ao abrir o arquivo ");Serial.print(nameFile);Serial.println("...");
  }
  stringJson="";
}
//Função para remover um arquivo existente
void deleteFileSdCard(String nameFile){
  SD.remove(nameFile);
}
//Função de leitura dos dados do módulo GPS
void connectGPS(){
  //Conexao com modulo GPS
  while (Serial3.available() > 0){
    char c = Serial3.read();
    gps.encode(c);
  }
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("GPS nao detectado, verificar ligacao");
    while (true);
  }else{
    connectIsValid = gps.location.isValid();
    if(connectIsValid){
      lati = gps.location.lat();
      lang = gps.location.lng();
    }
  }
}
//Coletando a hora e formatando do módulo GPS
void printDateTime(TinyGPSTime &t){
  if (!t.isValid()){
    Serial.println("Hora GPS invalida...");
  }else{
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    horaGPS = sz;
    horaGPS.trim();
  }
}
//Imprime dados GPS na serial e grava no arquivo gps.txt
void printAndWritePositionGPS(){
  int inicio = 0;
  String jsonGPS = "";
  printDateTime(gps.time);
  jsonGPS = createJsonGps();
  if(connectIsValid){
    writeSdCard(jsonGPS, "gps.txt");
    sizeJsonGps = jsonGPS.length();
    for (int x = inicio; x < sizeJsonGps; x++){
     EEPROM.write(x, byte(jsonGPS.charAt(x)));
    }
    Serial.println(jsonGPS);
    Serial.println();
  }else{
    Serial.println("Ainda sem posicao valida");
  }  
}
//Le arquivo temp.txt e imprime na serial
void printAndReadPositionGPS(){
  int x = 0;
  char c;
  do{
    c = char(EEPROM.read(x));
    Serial.print(c);
    x++;
  }while(c != '}');
   Serial.println("GPS############################################");
}
//Imprime dados temperatura na serial e grava no arquivo temp.txt
void printAndWriteTemperature(){
  String jsonTEMP = "";
  int j = 0;
  printDateTime(gps.time);
  jsonTEMP = createJsonTemp(temperatura());
  writeSdCard(jsonTEMP, "temp.txt");
  sizeJsonTemp = jsonTEMP.length() + 200;
  for (int x = 200; x < sizeJsonTemp; x++){
   EEPROM.write(x, byte(jsonTEMP.charAt(j)));
   j++;
  }
  Serial.println(jsonTEMP);
  Serial.println();
}
//Le arquivo temp.txt e imprime na serial
void printAndReadTemperature(){
int x = 200;
  char c;
  do{
    c = char(EEPROM.read(x));
    Serial.print(c);
    x++;
  }while(c != '}');
   Serial.println("TEMP############################################");
}
//Imprime dados radiação na serial e grava no arquivo rad.txt
void printAndWriteRaiation(){
  String jsonRAD = "";
  int j = 0;
  printDateTime(gps.time);
  jsonRAD = createJsonRad(rad());
  writeSdCard(jsonRAD, "rad.txt");
  sizeJsonRad = jsonRAD.length() + 400;
  for (int x = 400; x < sizeJsonRad; x++){
   EEPROM.write(x, byte(jsonRAD.charAt(j)));
   j++;
  }
  Serial.println(jsonRAD);
  Serial.println();
}
//Le arquivo rad.txt e imprime na serial
void printAndReadRaiation(){
int x = 400;
  char c;
  do{
    c = char(EEPROM.read(x));
    Serial.print(c);
    x++;
  }while(c != '}');
   Serial.println("RAD############################################");
}
//Imprime dados monóxido na serial e grava no arquivo mon.txt
void printAndWriteMon(){
  String jsonMON = "";
  int j = 0;
  printDateTime(gps.time);
  jsonMON = createJsonMon(monox());
  writeSdCard(jsonMON, "mon.txt");
  sizeJsonMon = jsonMON.length() + 600;
  for (int x = 600; x < sizeJsonMon; x++){
   EEPROM.write(x, byte(jsonMON.charAt(j)));
   j++;
  }
  Serial.println(jsonMON);
  Serial.println();
}
//Le arquivo mon.txt e imprime na serial
void printAndReadMon(){
  int x = 600;
  char c;
  do{
    c = char(EEPROM.read(x));
    Serial.print(c);
    x++;
  }while(c != '}');
   Serial.println("MON############################################");
}
//Imprime dados monóxido na serial e grava no arquivo chuva.txt
void printAndWriteChuva(){
  String jsonCHUVA = "";
  int j = 0;
  printDateTime(gps.time);
  jsonCHUVA = createJsonChuva(chuva());
  writeSdCard(jsonCHUVA, "chuva.txt");
  sizeJsonChuva = jsonCHUVA.length() + 800;
  for (int x = 800; x < sizeJsonChuva; x++){
   EEPROM.write(x, byte(jsonCHUVA.charAt(j)));
   j++;
  }
  Serial.println(jsonCHUVA);
  Serial.println();
}
//Le arquivo chuva.txt e imprime na serial
void printAndReadChuva(){
  int x = 800;
  char c;
  do{
    c = char(EEPROM.read(x));
    Serial.print(c);
    x++;
  }while(c != '}');
   Serial.println("CHUVA############################################");
}
//Imprime dados gerais na serial e grava no arquivo dados.txt
void printAndWriteDados(){
  String jsonDADOS = "";
  int j = 0;
  printDateTime(gps.time);
  jsonDADOS = createJsonDados(rad(), temperatura(), monox(), chuva());
  if(connectIsValid){
    writeSdCard(jsonDADOS, "dados.txt");
    sizeJsonDados = jsonDADOS.length() + 1000;
    for (int x = 1000; x < sizeJsonDados; x++){
     EEPROM.write(x, byte(jsonDADOS.charAt(j)));
     j++;
    }
    Serial.println(jsonDADOS);
    Serial.println();
  }else{
    Serial.println("Ainda dados sem posicao valida");
  }  
}
//Le arquivo dados.txt e imprime na serial
void printAndReadDados(){
  int x = 1000;
  char c;
  do{
    c = char(EEPROM.read(x));
    Serial.print(c);
    x++;
  }while(c != '}');
   Serial.println("DADOS############################################");
}
//

//Loop principal
void loop() {
//{"TYPE":"ENABLE","PROTOTYPE":2,"status":1}
//{"TYPE":"CONFIG","PROTOTYPE":2,"radiacao":5000,"temperatura":5000,"monoxido":5000,"gps":5000,"chuva":5000}
//asm volatile ("  jmp 0"); 
  //Da início as threads
  String inputData="";
  String jsonTemp="";
  //Star na funções periódicas
  controller.run(); 
  connectGPS();
  
/*Codigo reservado
  if(Serial.available()){
    while(Serial.available()){
      char caracter = Serial.read();
      inputData += caracter;
      delay(10);
    } 
    jsonTemp="";
    inputData="";
    inputTemp="";
  }*/
  
}
//Converte String x JSON
String createJsonRecive(String inputData){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(inputData);
  configAux = "";
  root.printTo(configAux);
  String st = root[String("TYPE")];
  return st;
}

//FUNÇÕES PARA A CRIAÇÃO DE JSON'S
//OBJETO JSON GPS
String createJsonGps(){
  DynamicJsonBuffer jsonBuffer;
  inputLatLang = "{\"TYPE\":\"GPS\",\"PROTOTYPE\":0,\"lat\":0,\"long\":0,\"hora\":\"0\"}";
  JsonObject& root = jsonBuffer.parseObject(inputLatLang);
  String json="";
  root[String("TYPE")] = "GPS";
  root[String("PROTOTYPE")] = 2;
  root[String("lat")] = lati;
  root[String("long")] = lang;
  root[String("hora")] = horaGPS;
  root.printTo(json);
  return json;
}
//OBJETO JSON TEMPERATURA
String createJsonTemp(float temp){
  DynamicJsonBuffer jsonBuffer;
  inputTemp = "{\"TYPE\":\"TEMPERATURA\",\"PROTOTYPE\":0,\"temperatura\":0,\"horaTemp\":\"0\"}";
  JsonObject& root = jsonBuffer.parseObject(inputTemp);
  String json="";
  root[String("TYPE")] = "TEMPERATURA";
  root[String("PROTOTYPE")] = 2;
  root[String("temperatura")] = temp;
  root[String("horaTemp")] = horaGPS;
  root.printTo(json);
  return json;
}
//OBJETO JSON RADIACAO
String createJsonRad(int rad){
  DynamicJsonBuffer jsonBuffer;
  inputRad = "{\"TYPE\":\"RADIACAO\",\"PROTOTYPE\":0,\"radiacao\":0,\"hora\":\"0\"}";
  JsonObject& root = jsonBuffer.parseObject(inputRad);
  String json="";
  root[String("TYPE")] = "RADIACAO";
  root[String("PROTOTYPE")] = 2;
  root[String("radiacao")] = rad;
  root[String("hora")] = horaGPS;
  root.printTo(json);
  return json;
}
//OBJETO JSON MONOXIDO DE CARBONO
String createJsonMon(int monox){
  DynamicJsonBuffer jsonBuffer;
  inputMon = "{\"TYPE\":\"MONOCARB\",\"PROTOTYPE\":0,\"monoxido\":0,\"hora\":\"0\"}";
  JsonObject& root = jsonBuffer.parseObject(inputMon);
  String json="";
  root[String("TYPE")] = "MONOCARB";
  root[String("PROTOTYPE")] = 2;
  root[String("monoxido")] = monox;
  root[String("hora")] = horaGPS;
  root.printTo(json);
  return json;
}
//OBJETO JSON CHUVA
String createJsonChuva(String chuva){
  DynamicJsonBuffer jsonBuffer;
  inpuChuva = "{\"TYPE\":\"CHUVA\",\"PROTOTYPE\":0,\"chuva\":\"0\",\"hora\":\"0\"}";
  JsonObject& root = jsonBuffer.parseObject(inpuChuva);
  String json="";
  root[String("TYPE")] = "CHUVA";
  root[String("PROTOTYPE")] = 2;
  root[String("chuva")] = 0;
  root[String("hora")] = horaGPS;
  root.printTo(json);
  return json;
}
//OBJETO JSON DADOS
String createJsonDados(int rad, float temp, int monox, String chuva){
  DynamicJsonBuffer jsonBuffer;
  inputDados = "{\"TYPE\":\"DADOS\",\"PROTOTYPE\":0,\"radiacao\":0,\"temperatura\":0,\"monoxido\":0,\"chuva\":0,\"lat\":0,\"lang\":0,\"hora\":\"0\"}";
  JsonObject& root = jsonBuffer.parseObject(inputDados);
  String json="";
  root[String("TYPE")] = "DADOS";
  root[String("PROTOTYPE")] = 2;
  root[String("radiacao")] = rad;
  root[String("temperatura")] = temp;
  root[String("monoxido")] = monox;
  root[String("chuva")] = chuva;
  root[String("lat")] = lati;
  root[String("lang")] = lang;
  root[String("hora")] = horaGPS;
  root.printTo(json);
  return json;
}
//FUNÇÕES PARA A CRIAÇÃO DE JSON'S DE CONFIGURAÇÕES
//OBJETO JSON ENABLE
String createJsonEnable(int statusPrototype){
  DynamicJsonBuffer jsonBuffer;
  confgEnable = "{\"TYPE\":\"ENABLE\",\"PROTOTYPE\":0,\"status\":-1}";
  JsonObject& root = jsonBuffer.parseObject(confgEnable);
  String json="";
  root[String("TYPE")] = "ENABLE";
  root[String("PROTOTYPE")] = 2;
  root[String("status")] = statusPrototype;
  root.printTo(json);
  return json;
}
//OBJETO JSON CONFIGURAÇÂO DE TAXAS DE ATUALIZAÇÃO DE LEITURA DOS DADOS
String createJsonConfig(int rad, int temp, int mon, int gps, int chuva, int dados){
  DynamicJsonBuffer jsonBuffer;
  configAtualizacao = "{\"TYPE\":\"CONFIG\",\"PROTOTYPE\":0,\"radiacao\":0,\"temperatura\":0,\"monoxido\":0,\"gps\":0,\"chuva\":0,\"dados\":0}";
  JsonObject& root = jsonBuffer.parseObject(configAtualizacao);
  String json="";
  root[String("TYPE")] = "CONFIG";
  root[String("PROTOTYPE")] = 2;
  root[String("radiacao")] = rad;
  root[String("temperatura")] = temp;
  root[String("monoxido")] = mon;
  root[String("gps")] = gps;
  root[String("chuva")] = chuva;
  root[String("dados")] = dados;
  root.printTo(json);
  return json;
}
//Funções de leitura dos sensores
//Função retorna um float de temperatura
float temperatura(){
  // Le a informacao do sensor
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(sensor1);
  return tempC;
}
//Função retorna uma string com o status da chuva
String chuva(){
  //int val_d = digitalRead(pino_d);
  int val_a = analogRead(pino_a);
  //Fornce o status da chuva
  if (val_a >900 && val_a <1023){
    return "Sem Chuva";
  }else if (val_a > 600 && val_a < 900){
    return "Fraca";
  }else if (val_a > 400 && val_a < 600){
    return "Moderada";
  }else if (val_a < 400){
    return "Forte";
  }    
}
//Função retorna um inteiro de monóxido de carbono
int monox(){
  int monox = analogRead(AOUTpin); //reads the analaog value from the CO sensor's AOUT pin
  return monox;
}
//Função retorna um inteiro de raiação ultravioleta
int rad(){
  int UV_index = 0;
  int valor_sensor = analogRead(pino_sensor);
  //Calcula tensao em milivolts
  int tensao = (valor_sensor * (5.0 / 1023.0)) * 1000;
  //Compara com valores tabela UV_Index
  if (tensao > 0 && tensao < 50){
    UV_index = 0;
  }else if (tensao > 50 && tensao <= 227){
    UV_index = 0;
  }else if (tensao > 227 && tensao <= 318){
    UV_index = 1;
  }else if (tensao > 318 && tensao <= 408){
    UV_index = 2;
  }else if (tensao > 408 && tensao <= 503){
    UV_index = 3;
  }else if (tensao > 503 && tensao <= 606){
    UV_index = 4;
  }else if (tensao > 606 && tensao <= 696){
    UV_index = 5;
  }else if (tensao > 696 && tensao <= 795){
    UV_index = 6;
  }else if (tensao > 795 && tensao <= 881){
    UV_index = 7;
  }else if (tensao > 881 && tensao <= 976){
    UV_index = 8;
  }else if (tensao > 976 && tensao <= 1079){
    UV_index = 9;
  }else if (tensao > 1079 && tensao <= 1170){
    UV_index = 10;
  }else if (tensao > 1170){
    UV_index = 11;
  }
  return UV_index;
}
