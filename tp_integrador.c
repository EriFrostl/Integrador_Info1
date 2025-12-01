#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

//REPO: https://github.com/EriFrostl/Integrador_Info1.git
//para compilar: gcc -std=c99 -Wall -Wextra -pedantic -O0 -g -o tp_i_final tp_integrador.c -lcurl


struct memory {
  char *response;
  size_t size;
};

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = nmemb;
  struct memory *mem = clientp;

  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
    return 0;  /* out of memory */

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

/*Estructura con los datos relevantes de los mensajes*/
struct info_mensaje{
  long update_id;
  long chat_id;
  char username[64];
  char text [512];
  long hora;
};

void buscar_token (char *);
void extraer_info (const char * const, struct info_mensaje*);
void registrar_mensajes (struct info_mensaje);
void arma_url_rta (struct info_mensaje, const char* token, char* url_respondedor);


int main(void) {

  char token[128];
  char url_base[256];
  char url[256];
  struct info_mensaje mensaje;
  int no_ser_denso=1;
  char url_sendMessage[256];

  srand(time(NULL));

  buscar_token(token);

  /*armo la URL base*/
  sprintf(url_base,"%s%s%s", "https://api.telegram.org/bot", token, "/getUpdates");
  sprintf(url, "%s", url_base);

  /*arranco el curl*/
  CURLcode res;
  CURL *curl = curl_easy_init();

 
  /*Entro en mi loop infinito*/
  while(1){
    struct memory chunk = {0};

    if(curl) {

      /*Busca actualizaciones*/
      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

      res = curl_easy_perform(curl);
      if (res != 0)
        printf("Error Código: %d\n", res);

      /*Cuando la needle no está en el haybale, strstr devuelve null*/
      if(strstr(chunk.response, "update_id") == NULL){
        if(no_ser_denso){
          printf("Todavía no hay mensajes\n");
          printf("%s\n", chunk.response);
          no_ser_denso--;
        }
        
        free(chunk.response);
        sleep(2);
        continue;
      }

      printf("%s\n", chunk.response);

      extraer_info(chunk.response, &mensaje);
      registrar_mensajes(mensaje);

      /*Revisor de errores

      printf("\n%ld", mensaje.update_id);
      printf("\n%ld", mensaje.chat_id);
      printf("\n%s", mensaje.username);
      printf("\n%s", mensaje.text);
      printf("\n%ld", mensaje.hora);

      */

      arma_url_rta(mensaje, token, url_sendMessage);

      free(chunk.response);

      /*Mando el mensaje*/
      struct memory chunk_send = {0};

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk_send);
      curl_easy_setopt(curl, CURLOPT_URL, url_sendMessage);

      res = curl_easy_perform(curl);

      if (res != 0) {
          printf("Error Código SEND: %d\n", res);
      } else if (chunk_send.response) {
          printf("RESPUESTA sendMessage: %s\n", chunk_send.response);
      }

      free(chunk_send.response);

      /*Actualizo URL*/
      sprintf(url,"%s?offset=%ld", url_base, mensaje.update_id + 1);


      sleep(2);
    }
    

  }

  curl_easy_cleanup(curl);
  
  return 0;
}



void buscar_token (char * token){
    
    char archivo_token[128];
    FILE *fp;
    int tok_valido;
    
    do{

        do{
            printf("Ingrese el nombre del archivo donde se encuentra el TOKEN:\n");
            scanf (" %127s", archivo_token);
            
            fp=fopen(archivo_token, "r");
        
            if (fp == NULL)
                printf("Error, ese archivo no se encuentra o no puede abrirse.\n");
        
        } while (fp == NULL);
        
        if (fscanf(fp, " %127s", token) != 1){
            printf("Error: el archivo no contiene un token válido.\n");
            tok_valido=0;
        } else
            tok_valido=1;
        
        token[strcspn(token, "\r\n")] = '\0';
    
        fclose (fp);
    }while (tok_valido==0);

}

/*En string existe strstr que busca una 'aguja'(cadena de char especpifica) en un 'pajar'(cadena)*/
void extraer_info (const char * const haystack, struct info_mensaje* msg){

  char *p1;
  char arr_temp[512]; /*tamano de lo mas grande que puedo tener, text*/
  int i=0,j=0;

  p1=strstr(haystack, "update_id");
  p1=strstr(p1, ":");
  p1++;

  while (*p1 != ',')
  {
    arr_temp[i++]= *p1;
    p1++;
  };
  arr_temp[i]='\0';

  /*atol es una funcion de stdlib que convierte un string en long*/
  msg->update_id=atol(arr_temp);
  

  p1=strstr(haystack, "chat");
  p1=strstr(p1, "id");
  p1+=4;
  i=0;

  while (*p1 != ',')
  {
    arr_temp[i++]= *p1;
    p1++;
  };
  arr_temp[i]='\0';

  msg->chat_id=atol(arr_temp);


  p1=strstr(haystack, "username");
  p1=strstr(p1, ":\"");
  p1+=2;
  i=0;

  while (*p1 != '"')
  {
    arr_temp[i++]= *p1;
    p1++;
  };
  arr_temp[i]='\0';

  for(j=0;j<i;j++){
    msg->username[j]=arr_temp[j];
  };
  msg->username[j]='\0';
  

  p1=strstr(haystack, "text");
  p1=strstr(p1, ":\"");
  p1+=2;
  i=0;

  while (*p1 != '"')
  {
    arr_temp[i++]= *p1;
    p1++;
  };
  arr_temp[i]='\0';

  for(j=0;j<i;j++){
    msg->text[j]=arr_temp[j];
  };
  msg->text[j]='\0';


/*ojo que upDATE tambien dice date, entonces lo busco con comillas*/
  p1=strstr(haystack, "\"date\"");
  p1=strstr(p1, ":");
  p1++;
  i=0;

  while (*p1 != ',')
  {
    arr_temp[i++]= *p1;
    p1++;
  };
  arr_temp[i]='\0';

  msg->hora=atol(arr_temp);

}


void registrar_mensajes (struct info_mensaje msg){
	
	FILE *fp= fopen("registro_mensajes.txt", "a");
	if (fp == NULL){
		printf("Error, el archivo no pudo se abrir/crear.\n");
    return;
	} /*aca podría haber metido un exit, pero elegí que mi programa siga,
    haciendole saber al usuario que hubo un problema
    y no cerrando el fp para no romper nada*/
	
	fprintf (fp, "%ld  %s  %s\n", msg.hora, msg.username, msg.text);
	
    fclose(fp);
}

void arma_url_rta (struct info_mensaje msg, const char* token, char* url_sendMessage){

  char mensaje[128];
  int h=0, c=0;
  char *no_entendi[]={
    "No%20entendí%20lo%20que%20dijiste",
    "Solo%20estoy%20programado%20para%20decir%20hola%20o%20chau",
    "Mejor%20preguntale%20a%20ChatGPT...",
    "No%20tengo%20una%20respuesta%20para%20eso",
    "Escuchá%20a%20tu%20corazón"
  };


  /*Veo si me dice hola*/
  if((strstr(msg.text, "Hola") != NULL) ||
      (strstr(msg.text, "hola") != NULL) ||
      (strstr(msg.text, "Buen dia") != NULL)||
      (strstr(msg.text, "Buenos dias") != NULL)){
    
        /*Dos veces el % para que me lo tome literal*/
        sprintf(mensaje,"%s%s", "Hola,%20", msg.username);
        h++;
 
  };
 
  /*Veo si me dice chau*/
  if((strstr(msg.text, "Chau") != NULL) ||
      (strstr(msg.text, "chau") != NULL) ||
      (strstr(msg.text, "Adios") != NULL)||
      (strstr(msg.text, "Hasta luego") != NULL)){
    
        sprintf(mensaje,"%s", "Chau,%20fue%20un%20gusto%20charlar%20con%20vos!");
        c++;
 
  };

  if ((h)&&(c))
    sprintf(mensaje,"%s", "Hola%20y%20chau");

  if (!((h)||(c))){
    sprintf(mensaje,"%s", no_entendi[rand()%5]);    
  };

  sprintf(url_sendMessage, "%s%s%s%ld%s%s", "https://api.telegram.org/bot", token, "/sendMessage?chat_id=", msg.chat_id, "&text=", mensaje);
}