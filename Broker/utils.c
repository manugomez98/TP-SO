/*
 * conexiones.c
 *
 *  Created on: 3 mar. 2019
 *      Author: utnso
 */

#include "utils.h"

void iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next)
					{
		if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;

		if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
			close(socket_servidor);
			continue;
		}
		break;
	}

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	while (1)
		esperar_cliente(socket_servidor);
}

void esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;

	u_int32_t tam_direccion = sizeof(struct sockaddr_in); //capaz no sea unsigned

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	if (socket_cliente != -1) {
		log_info(logger, "Se ha conectado un proceso al servidor (Socket: %d)\n", socket_cliente);
	}

	pthread_create(&thread, NULL, (void*) serve_client, &socket_cliente);
	pthread_join(thread, NULL);

}

void serve_client(int* socket)
{
	int cod_op;
	if (recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
		cod_op = -1;
	process_request(cod_op, *socket);
}

void process_request(int cod_op, int cliente_fd) {
	u_int32_t id_mensaje;

	switch (cod_op) {
			case NEW_POKEMON:
			case APPEARED_POKEMON:
			case CATCH_POKEMON:
			case CAUGHT_POKEMON:
			case GET_POKEMON:
			case LOCALIZED_POKEMON:
			log_info(logger, "Se ha recibido un mensaje del tipo %d(%s).\n", cod_op, obtener_tipo_mensaje_string(cod_op));

			int32_t size;
			void* stream = (void*) recibir_cadena(cliente_fd, &size);


			id_mensaje = generar_id_mensaje();

			log_warning(logger_auxiliar,"La id recibida fue: %d\n", id_mensaje);

			uint32_t id_correlativo = 0;

			void* stream_a_agregar;

			if (es_cola_correlativa(cod_op)) {
				size -= sizeof(uint32_t);
				memcpy(&id_correlativo, stream, sizeof(uint32_t));
				stream_a_agregar = stream + sizeof(uint32_t);
			} else {
				stream_a_agregar = stream;
			}

			if(size > memoria->tamanio){
				log_warning(logger_auxiliar, "Se ha descartado el mensaje por ser demadiado grande (Como Boquita)\n");
				free(stream);
				pthread_exit(NULL);
			}

			t_particion* particion_auxiliar;
			if(id_correlativo != 0){
				for(uint32_t i = 0; i <list_size(memoria->particiones); i++){
					particion_auxiliar = list_get(memoria->particiones, i);
					if(particion_auxiliar->atributos->id_correlativo == id_correlativo){
					log_warning(logger_auxiliar, "Se ha descartado el mensaje por tener ID correlativo repetido\n");
					free(stream);
					pthread_exit(NULL);
					}
				}
			}




			t_atributos_particion* atributos = crear_atributos_particion(timer_lru++, cod_op, id_mensaje, id_correlativo, size);
			t_particion* particion_agregada = agregar_stream(memoria, stream_a_agregar, size, obtener_id_particion_libre(algoritmo_particion_libre), atributos, true);

			t_paquete* paquete = generar_paquete(particion_agregada);

			enviar_mensaje(paquete, cliente_fd);
			enviar_a_suscriptores(particion_agregada, obtener_lista_suscriptores(cod_op));

			log_warning(logger_auxiliar,"Mande todo\n" );
			destruir_paquete(paquete);
			free(stream);
			break;


		case SUSCRIPTOR:

			size = recibir_entero(cliente_fd);

			int32_t cola_size;
			char* cola_nombre = recibir_string(cliente_fd, &cola_size);

			u_int32_t id_suscriptor = recibir_entero(cliente_fd);

			if (id_suscriptor == 0) {
				agregar_suscriptor(cliente_fd, obtener_tipo_mensaje(cola_nombre));
			} else {
				actualizar_suscriptor(cliente_fd, obtener_tipo_mensaje(cola_nombre), id_suscriptor);
			}

			free(cola_nombre);
			break;


		case CONFIRMAR:

			size = recibir_entero(cliente_fd);
			u_int32_t id_cola_mensajes = recibir_entero(cliente_fd);
			id_mensaje = recibir_entero(cliente_fd);
			u_int32_t id_suscriptor_confirmado = recibir_entero(cliente_fd);
			t_particion* particion_confirmada = buscar_particion(memoria, id_mensaje);
			if (particion_confirmada == NULL)
				break;
			t_suscriptor* suscriptor_confirmado = buscar_suscriptor(particion_confirmada->atributos->suscriptores_enviados, id_suscriptor_confirmado);

			list_add(particion_confirmada->atributos->suscriptores_confirmados, suscriptor_confirmado);

			/*if(list_size(particion_confirmada->atributos->suscriptores_confirmados) == list_size(particion_confirmada->atributos->suscriptores_enviados)){
				log_warning(logger_auxiliar, "Se ha liberado el mensaje %d por haber sido confirmado por todos los suscriptores a los que se envio.\n", particion_confirmada->atributos->id_mensaje);
				particion_confirmada->ocupada = false;
			}*/

			log_info(logger, "El suscriptor %d confirmo confirm?? haber recibido el mensaje %d de la cola %d (%s).\n", id_suscriptor_confirmado, id_mensaje, id_cola_mensajes, obtener_tipo_mensaje_string(id_cola_mensajes));

			break;

		case 0:
			pthread_exit(NULL);
		case -1:
			pthread_exit(NULL);
	}
}

int recibir_entero(int socket_cliente) {

	int entero;

	recv(socket_cliente, &entero, sizeof(int), MSG_WAITALL);

	return entero;
}

void* recibir_cadena(int socket_cliente, int* size)
{
	void* cadena;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	cadena = malloc(*size);
	recv(socket_cliente, cadena, *size, MSG_WAITALL);
	//printf("Cadena: %s\n", (char*) cadena + 4 );
	return cadena;
}

char* recibir_string(int socket_cliente, u_int32_t* size)
{
	char * cadena;

	recv(socket_cliente, size, sizeof(u_int32_t), MSG_WAITALL);
	cadena = malloc((*size) + 1);
	recv(socket_cliente, cadena, *size, MSG_WAITALL);
	cadena[(*size)] = '\0';
	return cadena;
}


tipo_mensaje obtener_tipo_mensaje(char* tipo) {
	if (string_equals_ignore_case(tipo, "NEW_POKEMON"))
		return NEW_POKEMON;
	if (string_equals_ignore_case(tipo, "APPEARED_POKEMON"))
		return APPEARED_POKEMON;
	if (string_equals_ignore_case(tipo, "CATCH_POKEMON"))
		return CATCH_POKEMON;
	if (string_equals_ignore_case(tipo, "CAUGHT_POKEMON"))
		return CAUGHT_POKEMON;
	if (string_equals_ignore_case(tipo, "GET_POKEMON"))
		return GET_POKEMON;
	if (string_equals_ignore_case(tipo, "LOCALIZED_POKEMON"))
		return LOCALIZED_POKEMON;
	if (string_equals_ignore_case(tipo, "CONFIRMAR"))
		return CONFIRMAR;
	return -1;
}

char* obtener_tipo_mensaje_string(tipo_mensaje tipo) {
	if (tipo == NEW_POKEMON)
		return "NEW_POKEMON";
	if (tipo == APPEARED_POKEMON)
		return "APPEARED_POKEMON";
	if (tipo == CATCH_POKEMON)
		return "CATCH_POKEMON";
	if (tipo == CAUGHT_POKEMON)
		return "CAUGHT_POKEMON";
	if (tipo == GET_POKEMON)
		return "GET_POKEMON";
	if (tipo == LOCALIZED_POKEMON)
		return "LOCALIZED";
	if (tipo == SUSCRIPTOR)
		return "SUSCRIPTOR";
	if (tipo == CONFIRMAR)
		return "CONFIRMAR";
	return "DESCONOCIDO";
}

void* serializar_paquete(t_paquete* paquete, size_t bytes) {
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->tipo_mensaje), sizeof(u_int32_t));
	desplazamiento += sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(paquete->id_mensaje), sizeof(u_int32_t));
	desplazamiento += sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(u_int32_t));
	desplazamiento += sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(paquete->id_correlativo), sizeof(u_int32_t));
	desplazamiento += sizeof(u_int32_t);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

void notificar_id_suscriptor(t_suscriptor* suscriptor, tipo_mensaje tipo_mensaje) { //envio al suscriptor un mensaje de argumentos (id, SUSCRIPTOR, cola)
	void* stream = malloc(sizeof(tipo_mensaje));
	memcpy(stream, &tipo_mensaje, sizeof(tipo_mensaje));
	t_buffer* buffer = crear_buffer(sizeof(tipo_mensaje), stream);
	t_paquete* paquete = crear_paquete(suscriptor->id, 0, SUSCRIPTOR, buffer);

	enviar_mensaje(paquete, suscriptor->numero_socket);
	free(stream);
	destruir_paquete(paquete);
}

u_int32_t enviar_mensaje(t_paquete* paquete, u_int32_t socket) {
	size_t bytes = paquete->buffer->size + 4 * sizeof(int32_t);
	void* a_enviar = serializar_paquete(paquete, bytes);
	uint32_t envio = send(socket, a_enviar, bytes, MSG_NOSIGNAL);
	free(a_enviar);
	return envio;
}

void enviar_a_suscriptor(t_particion* particion, t_suscriptor* suscriptor) {

	t_paquete* paquete = generar_paquete(particion);

	uint32_t envio = enviar_mensaje(paquete, suscriptor->numero_socket);

	list_add(particion->atributos->suscriptores_enviados, suscriptor);
	log_info(logger, "Se envio el mensaje de ID %d al suscriptor %d. (Socket: %d)\n", particion->atributos->id_mensaje, suscriptor->id, envio);
	//esperar_confirmacion(suscriptor);

	destruir_paquete(paquete);
}

void esperar_confirmacion(t_suscriptor* suscriptor) {
	pthread_t nuevo;
	pthread_create(&nuevo, NULL, (void*) serve_client, &suscriptor->numero_socket);
	pthread_join(nuevo, NULL);
}

void enviar_a_suscriptores(t_particion* particion, t_list* suscriptores) {
	for (u_int32_t i = 0; i < list_size(suscriptores); i++) {
		t_suscriptor* suscriptor = (void*) list_get(suscriptores, i);
		enviar_a_suscriptor(particion, suscriptor);
		list_add(particion->atributos->suscriptores_enviados, suscriptor);
	}

}

void agregar_suscriptor(u_int32_t socket, uint32_t id_cola_mensajes) {

	t_suscriptor* suscriptor = generar_suscriptor(socket, id_cola_mensajes);
	notificar_id_suscriptor(suscriptor, id_cola_mensajes);

	list_add(obtener_lista_suscriptores(id_cola_mensajes), suscriptor);
	log_info(logger, "El suscriptor %d ha sido agregado a la cola %d (%s).\n", suscriptor->id, id_cola_mensajes, obtener_tipo_mensaje_string(id_cola_mensajes));

	t_list* particiones = memoria->particiones;
	t_particion* particion_actual;

	for (int i = 0; i < list_size(particiones); i++) {
		particion_actual = list_get(particiones, i);
		if (particion_actual->atributos->cola_mensajes == id_cola_mensajes && particion_actual->ocupada) {
			enviar_a_suscriptor(particion_actual, suscriptor);
			particion_actual->atributos->lru = timer_lru++;
		}
	}
}

t_suscriptor* actualizar_suscriptor(u_int32_t socket, uint32_t id_cola_mensajes, u_int32_t id_suscriptor) {

	t_suscriptor* suscriptor = buscar_suscriptor(obtener_lista_suscriptores(id_cola_mensajes), id_suscriptor);
	suscriptor->numero_socket = socket;

	t_list* particiones = memoria->particiones;

	t_particion* particion_actual;
	;

	for (uint32_t i = 0; i < list_size(particiones); i++) {
		particion_actual = list_get(particiones, i);
		if (particion_actual->atributos->cola_mensajes == id_cola_mensajes && particion_actual->ocupada) {
			if (!confirmo_mensaje(suscriptor, particion_actual)) {
				enviar_a_suscriptor(particion_actual, suscriptor);
				particion_actual->atributos->lru = timer_lru++;
			}
		}
	}

	return suscriptor;

}

bool confirmo_mensaje(t_suscriptor* suscriptor, t_particion* particion) {
	t_list* confirmados = particion->atributos->suscriptores_confirmados;
	for (uint32_t i = 0; i < list_size(confirmados); i++) {
		if (suscriptor == list_get(confirmados, i))
			return true;
	}
	return false;
}

bool es_cola_correlativa(tipo_mensaje tipo) {
	return tipo == 2 || tipo == 4 || tipo == 6;
}

void finalizar_servidor() {
	//generar_dump(memoria);

	for (uint32_t i = 0; i < 6; i++) {
		list_destroy_and_destroy_elements(lista_suscriptores[i], destruir_suscriptor);
	}
	destruir_memoria(memoria);
	log_destroy(logger);
	config_destroy(config);
	exit(0);
}
