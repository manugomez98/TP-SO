/*
 * utils.h
 *
 *  Created on: 7 jun. 2020
 *      Author: utnso
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#define IP "127.0.0.2"
#define PUERTO "37228"

typedef enum{
	NEW_POKEMON = 1,
	APPEARED_POKEMON = 2,
	CATCH_POKEMON = 3,
	CAUGHT_POKEMON = 4,
	GET_POKEMON = 5,
	LOCALIZED_POKEMON = 6,
	SUSCRIPTOR = 7,
}tipo_mensaje;

pthread_t thread;
u_int32_t id_cola_get;
u_int32_t id_cola_new;
u_int32_t id_cola_catch;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	tipo_mensaje codigo_operacion;
	t_buffer* buffer;
} t_paquete;

tipo_mensaje obtener_tipo_mensaje(char* tipo);
char* obtener_tipo_mensaje_string(tipo_mensaje tipo);
void* recibir_cadena(int socket_cliente, int* size);
void iniciar_servidor(void);
void esperar_cliente(int socket_servidor);
void serve_client(int* socket);
void process_request(int cod_op, int cliente_fd);
u_int32_t recibir_entero(int socket_cliente);
void* serializar_paquete(t_paquete* paquete, u_int32_t *bytes);
int crear_conexion(char *ip, char* puerto);
void enviar_mensaje(char* argv[], u_int32_t socket_cliente);
void agregar_string(int* offset, char* string, void** stream);
void agregar_entero(int* offset, char* string, void** stream);
void* generar_stream(char** argumentos, t_paquete* paquete);
u_int32_t obtener_size(char* argumentos[], tipo_mensaje tipo);
void liberar_conexion(u_int32_t socket_cliente);
void asignar_id_cola_de_mensajes(u_int32_t id_a_asignar, tipo_mensaje tipo);


#endif /* UTILS_H_ */