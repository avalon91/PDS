#include <SPI.h>
#include <Ethernet.h>
#include <sha1.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <mysql.h>
#include <SPI.h>
#include <MFRC522.h>

byte mac_addr[] = {0x90, 0xA2, 0xDA, 0x00, 0xC9, 0xB9}; //endereço MAC do arduino
IPAddress server_addr(192,168,43,113);		//endereço de IP do Raspberry Pi
char user[] = "arduino";				//nome de usuário para acessar a database
char password[] = "senha";				//senha do usuário

Connector my_conn; 						//cria um objeto da conexão, da biblioteca

char query[75];
const char INSERT_DATA[] = "INSERT INTO dbteste.log VALUES ('";
//string contendo o início do comando de inserção de dados na tabela 'log', dentro da database 'dbteste'. Os valores são:
//data e hora (variável do tipo DATETIME), inteiro, inteiro, float

const char TEST_SELECT_QUERY[] = "SELECT * FROM dbteste.log";
//string contendo o comando de seleção dos todos os dados da tabela 'log', dentro da database 'dbteste'

void setup(){
	Ethernet.begin(mac_addr);
  	Serial.begin(9600);
  	while (!Serial) {
   	 ; // wait for serial port to connect. Needed for Leonardo only
  	}
  	delay(1000);
  	Serial.println("Connecting...");
  	if (my_conn.mysql_connect(server_addr, 3306, user, password)) {
    	delay(1000);
  	}
  	else
    	Serial.println("Connection failed.");

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
}

void loop(){
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