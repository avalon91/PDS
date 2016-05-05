#include <SPI.h>
#include <Ethernet.h>
#include <sha1.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <mysql.h>
#include <MFRC522.h>
#include <Wire.h>

#define enderecoI2C 0x68

#define SS_PIN 8
#define RST_PIN 9

byte segundo, minuto, hora, diaDaSemana, diaDoMes, mes, ano;

bool flag = false;

MFRC522 mfrc522(SS_PIN, RST_PIN);

byte mac_addr[] = {0x90, 0xA2, 0xDA, 0x00, 0xC9, 0xB9}; //endereço MAC do arduino
IPAddress server_addr(192,168,0,106);		//endereço de IP do Raspberry Pi
char user[] = "arduino";				//nome de usuário para acessar a database
char password[] = "senha";				//senha do usuário

Connector my_conn; 						//cria um objeto da conexão, da biblioteca

String tag = "";

char query[75];
const char INSERT_DATA[] = "INSERT INTO dbteste.log VALUES ('";
//string contendo o início do comando de inserção de dados na tabela 'log', dentro da database 'dbteste'. Os valores são:
//data e hora (variável do tipo DATETIME), inteiro, inteiro, float

const char TEST_SELECT_QUERY[] = "SELECT * FROM dbteste.log";
//string contendo o comando de seleção dos todos os dados da tabela 'log', dentro da database 'dbteste'

void setup(){
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  Ethernet.begin(mac_addr);
	Serial.begin(9600);
	while (!Serial) {
 	 ; // wait for serial port to connect. Needed for Leonardo only
	}
	delay(1000);
	Serial.println("Conectando a database...");
	if (my_conn.mysql_connect(server_addr, 3306, user, password)) {
  	delay(1000);
	}
	else
  	Serial.println("Conexao falhou.");
  
  /*SIMULAÇÃO
  strcat(query, INSERT_DATA); 		//concatenação da string 'INSERT_DATA' na string 'query'. Mais informações em: http://www.tutorialspoint.com/c_standard_library/c_function_strcat.htm
  char data[] = "2016-04-22 21:27:09";	//simulação de obtenção de hora e data a partir de chip RTC
  strcat(query, data);			//concatenação do primeiro valor a ser passado para a database
  strcat(query,"', ");		//string até o momento: "INSERT INTO dbteste.log VALUES('2016-04-22 21:27:09', "

  int int1 = 122;				//simulação da obtenção de um valor inteiro
  char int1c[12];				//buffer necessário par a conversão de inteiro para string
  itoa(int1, int1c, 10);		//conversão de inteiro para string. Mais informações em: http://playground.arduino.cc/Code/PrintingNumbers
  strcat(query, int1c);		//concatenação
  strcat(query, ", ");		//string até o momento: "INSERT INTO dbteste.log VALUES('2016-04-22 21:27:09', 122, "

  int int2 = 1020;			//simulação da obtenção de um segundo valor inteiro
  char int2c[12];				//buffer
  itoa(int2, int2c, 10);		//conversão
  strcat(query, int2c);		//concatenação
  strcat(query, ", ");		//string até o momento: "INSERT INTO dbteste.log VALUES('2016-04-22 21:27:09', 122, 1020, "

  //exemplo de leitura de um sensor, transformação de float para string (com a quantidade de casas especificadas nos argumentos da função)
  //mais informações aqui: http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__avr__stdlib_1ga060c998e77fb5fc0d3168b3ce8771d42.html
  //após a conversão, ocorre a inserção da string 'temperature' na string 'query'
  //depois da inserção dos dados na tabela, ocorre a impressão de todos os dados da tabela através da função 'do_query'

  float value_read = 30.2;
  char temperature[5];
  dtostrf(value_read, 1, 1, temperature);
  strcat(query, temperature);
  strcat(query, ")");
  Serial.println(query);
  my_conn.cmd_query(query);
  do_query(TEST_SELECT_QUERY);
  */
}

void loop(){
  tag = readRFID();
  if(flag == true){
    char tagC[12];
    leHora();
    tag.toCharArray(tagC, sizeof(unsigned int));
    strcat(query, tagC);
    strcat(query, ")");
    Serial.println(query);
    my_conn.cmd_query(query);
    flag = false;
    delay(1000);
  }
}

String readRFID(){
  flag = true;
  if (mfrc522.PICC_IsNewCardPresent()){
    if (mfrc522.PICC_ReadCardSerial()){
      //Mostra UID na serial
      Serial.print("UID da tag :");
      String conteudo= "";
      byte letra;
      for (byte i = 0; i < mfrc522.uid.size; i++){    
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], DEC);
        conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : ", "));
        conteudo.concat(String(mfrc522.uid.uidByte[i], DEC));
      }
      return conteudo;
    }
  }
}

void leHora(){
  String ajustaSegundo;
  String ajustaMinuto;
  String ajustaHora;
  String ajustaDiaDoMes;
  String ajustaMes;

  char horaC[12], minC[12], segC[12], diaC[12], mesC[12];

  Wire.beginTransmission(enderecoI2C); //Abrindo o modo I2C no modo de gravação.
  Wire.write((byte)0x00); //Redefinindo o ponteiro para o primeiro registro (0x00).
  Wire.endTransmission(); //finalizando o modo de gravação.
  Wire.requestFrom(enderecoI2C, 7);    //Abrindo o modo I2C no modo de leitura.

  segundo = bcdToDec(Wire.read() & 0x7f); //Alguns dados precisam de máscaras antes 
                                          //da conversão porque certos bits são 
                                          //bits de controle. 
  minuto = bcdToDec(Wire.read());         //convertendo os minutos. 
  hora = bcdToDec(Wire.read() & 0x3f);    //Alguns dados precisam de máscaras antes 
                                          //da conversão porque certos bits são 
                                          //bits de controle. Essa máscara define o
                                          //relógio para trabalhar no modo de 24h.
  diaDaSemana = bcdToDec(Wire.read());    //dia da semana, onde domingo começa com "0".
  diaDoMes = bcdToDec(Wire.read());       //convertendo o dia do mês.
  mes = bcdToDec(Wire.read());            //convertendo o mês.
  ano = bcdToDec(Wire.read());            //convertendo o ano.
  ajustaHora += ajustaZero(hora);
  ajustaMinuto += ajustaZero(minuto);
  ajustaSegundo += ajustaZero(segundo);
  ajustaDiaDoMes += ajustaZero(diaDoMes);
  ajustaMes += ajustaZero(mes);
  char anoC[12];
  itoa((int)ano, anoC, 10);
  strcat(query, anoC);
  strcat(query, "-");
  ajustaMes.toCharArray(mesC, sizeof(unsigned int));
  strcat(query, mesC);
  strcat(query, "-");
  ajustaDiaDoMes.toCharArray(diaC, sizeof(unsigned int));
  strcat(query, diaC);
  strcat(query, " ");
  ajustaHora.toCharArray(horaC, sizeof(unsigned int));
  strcat(query, horaC);
  strcat(query, ":");
  ajustaMinuto.toCharArray(minC, sizeof(unsigned int));
  strcat(query, minC);
  strcat(query, ":");
  ajustaSegundo.toCharArray(segC, sizeof(unsigned int));
  strcat(query, segC);
  strcat(query, "'");
}

void do_query(const char *q) {
  column_names *c; // pointer to column values
  row_values *r;   // pointer to row values

  // First, execute query. If it returns a value pointer,
  // we have a result set to process. If not, we exit.
  if (!my_conn.cmd_query(q)) {
    return;
  }

  // Next, we read the column names and display them.
  //
  // NOTICE: You must *always* read the column names even if
  //         you do not use them. This is so the connector can
  //         read the data out of the buffer. Row data follows the
  //         column data and thus must be read first.
  c = my_conn.get_columns();
  for (int i = 0; i < c->num_fields; i++) {
    Serial.print(c->fields[i]->name);
    if (i < c->num_fields - 1) {
      Serial.print(",");
    }
  }
  Serial.println();

  // Next, we use the get_next_row() iterator and read rows printing
  // the values returned until the get_next_row() returns NULL.
  int num_cols = c->num_fields;
  int rows = 0;
  do {
    r = my_conn.get_next_row();
    if (r) {
      rows++;
      for (int i = 0; i < num_cols; i++) {
        Serial.print(r->values[i]);
        if (i < num_cols - 1) {
          Serial.print(", ");
        }
      }
      Serial.println();
      // Note: we free the row read to free the memory allocated for it.
      // You should do this after you've processed the row.
      my_conn.free_row_buffer();
    }
  } while (r);
  Serial.print(rows);
  Serial.println(" rows in result.");

  // Finally, we are done so we free the column buffers
  my_conn.free_columns_buffer();
}

byte decToBcd(byte val){
  return ( (val/10*16) + (val%10) );
}
 
/* Função que realiza uma conversão de Binário para Decimal.
   Utilizada na impressão dos dados na tela do Monitor Serial.
*/
byte bcdToDec(byte val){
  return ( (val/16*10) + (val%16) );
}

/* Essa função insere o dígito "0" à esquerda dos dados gravados no módulo com 
   apenas um dígito, já que os valores menores que 10 são armazenados no módulo
   com apenas um dígito.  
*/
String ajustaZero(byte dado){
  String dadoAjustado;
  if (dado < 10) 
  {
    dadoAjustado += "0";  //concatena o dígito "0" ao valor da variável.
  }
  dadoAjustado += dado;   //concatena o dígito "0" ao valor do dado.
  
  return dadoAjustado;    //retorna o valor do dado ajustado.
}