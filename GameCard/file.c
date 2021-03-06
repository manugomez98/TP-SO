/*
 * file.c
 *
 *  Created on: 20 jun. 2020
 *      Author: utnso
 */
#include "file.h"

void actualizar_bit_map() {

	FILE* bitmap_file = fopen(archivo_bitmap_path, "w");
	verificar_validez_de_path(bitmap_file);
	fwrite(bitmap->bitarray, metadata_general->blocks / 8, 1, bitmap_file);
	fclose(bitmap_file);

	/*FILE* bitmap_file = fopen(archivo_bitmap_path, "wb+");
	 fwrite(bitmap, metadata_general->blocks / 8, 1, bitmap_file);
	 fclose(bitmap_file);
	 printf("Bit 0:%d --------------\n", bitarray_test_bit(bitmap, 0));
	 printf("YA ACTUALICE EL BITMAP--------------\n");*/
}

char* generar_pokemon_file_path(char* pokemon) {
	char* path_beta = string_new();
	string_append_with_format(&path_beta, "/Files/%s", pokemon);
	char* path = generar_nombre(path_beta);
	free(path_beta);
	return path;
}

void verificar_existencia_de_archivo(char* pokemon) {
	char* path_pokemon_file = generar_pokemon_file_path(pokemon);
	DIR* directorio_pokemon = opendir(path_pokemon_file);
	if (directorio_pokemon == NULL) {
		mkdir(path_pokemon_file, 0777);
		generar_metadata_bin(pokemon);
	}
	free(path_pokemon_file);
	closedir(directorio_pokemon);
}

void verificar_existencia_de_carpeta(char* nombre) {
	char* path_directorio = generar_nombre(nombre);
	DIR* directorio_pokemon = opendir(path_directorio);
	if (directorio_pokemon == NULL) {
		mkdir(path_directorio, 0777);
	}
	free(path_directorio);
	closedir(directorio_pokemon);
}

bool esta_abierto(FILE* file) {
	fseek(file, -1, SEEK_END);
	char abierto = (char) fgetc(file);
	return (abierto == 'Y');
}

void abrir_file(FILE* file) {
	fseek(file, -sizeof(char), SEEK_END);
	fputc('Y', file);
}

void cerrar_file(FILE* file) {
	fseek(file, -sizeof(char), SEEK_END);
	fputc('N', file);
}

void retroceder_hasta(char caracter_de_paro, FILE** file) {
	char caracter = fgetc(*file);
	while (caracter != caracter_de_paro) {
		fseek(*file, -2, SEEK_CUR);
		caracter = fgetc(*file);
	}
}

void avanzar_hasta(char caracter_de_paro, FILE** file) {
	char caracter = fgetc(*file);
	while (caracter != caracter_de_paro) {
		caracter = fgetc(*file);
	}
}

char* guardar_hasta(char caracter_de_paro, FILE** file) {
	char caracter = fgetc(*file);
	char* cadena_guardada = string_new();
	while (caracter != caracter_de_paro) {
		string_append_with_format(&cadena_guardada, "%c", caracter);
		caracter = fgetc(*file);
	}
	return cadena_guardada;
}

char* guardar_hasta_EOF(FILE** file) {
	char caracter = fgetc(*file);
	char* cadena_guardada = string_new();
	while (caracter != EOF) {
		string_append_with_format(&cadena_guardada, "%c", caracter);
		caracter = fgetc(*file);
	}
	return cadena_guardada;
}

char* guardar_hasta_con_EOF(char caracter_de_paro, FILE** file,
bool* llego_al_final) {
	char caracter = fgetc(*file);
	char* cadena_guardada = string_new();
	while (caracter != caracter_de_paro && caracter != EOF) {
		string_append_with_format(&cadena_guardada, "%c", caracter);
		caracter = fgetc(*file);
	}
	(*llego_al_final) = (caracter == EOF);
	if (*llego_al_final)
		fseek(*file, -1, SEEK_CUR);
	return cadena_guardada;
}

void verificar_estado_de_apertura_de_archivo_pokemon(FILE* file) {
	while(esta_abierto(file))
		sleep(config_gamecard->tiempo_de_reintento_operacion);
		//cambiar por sem??foro

	abrir_file(file);
	//alarm(config_gamecard->tiempo_de_reintento_operacion);
}

char* obtener_ultimo_bloque(FILE* file) {
	fseek(file, -2, SEEK_END);
	char bloque = fgetc(file);
	while ((bloque != ',') && (bloque != '[')) {
		fseek(file, -2, SEEK_CUR);
		bloque = fgetc(file);
	}
	char* ultimo_bloque = string_new();
	bloque = fgetc(file);
	while (bloque != ']') {
		string_append_with_format(&ultimo_bloque, "%c", bloque);
		bloque = fgetc(file);
	}
	//printf("Ultimo bloque: %s\n", ultimo_bloque);

	return ultimo_bloque;
}

char* obtener_directorio_block_path() {
	char* path_beta = string_new();
	string_append(&path_beta, "/Blocks");
	char* path = generar_nombre(path_beta);
	free(path_beta);
	return path;
}

char* obtener_bloque_path(char* bloque) {
	char* path = obtener_directorio_block_path();
	string_append_with_format(&path, "/%s.bin", bloque);
	return path;
}

char* obtener_bloque_disponible() {
	bool encontrado = false;
	int bloque_disponible = -1;
	int cantidad_total_bloques = metadata_general->blocks;

	for (int i = 0; (i < cantidad_total_bloques) && (!encontrado); i++) {
		encontrado = !(bitarray_test_bit(bitmap, i));
		if (encontrado) {
			bitarray_set_bit(bitmap, i);
			actualizar_bit_map();
			bloque_disponible = i + 1;
		}
	}
	if(bloque_disponible < 0) exit(1);
	return string_itoa(bloque_disponible);
}

uint32_t file_size(FILE* file) {
	uint32_t contador = 0;
	fseek(file, 0, SEEK_SET);
	char caracter = fgetc(file);
	while (caracter != EOF) {
		contador++;
		caracter = fgetc(file);
	}
	return contador;
}

void guardar_posterior(FILE* file, char** posterior) {
	char caracter = fgetc(file);
	while (caracter != '\n') {
		caracter = fgetc(file);
	}

	caracter = fgetc(file);
	while (caracter != EOF) {
		string_append_with_format(posterior, "%c", caracter);
		caracter = fgetc(file);
	}
}

uint32_t obtener_file_size(FILE* file, t_list* bloques_file){
	FILE* bloque_file = NULL;
	uint32_t size_total = 0;
	posicionar_en_inicio(bloques_file, bloque_file);
	for(int i=0; i<list_size(bloques_file); i++){
		bloque_file = list_get(bloques_file, i);
		size_total += file_size(bloque_file);
	}
	posicionar_en_inicio(bloques_file, bloque_file);

	return size_total;
}

void actualizar_size(FILE* file, t_list* bloques_file) {
	char* posterior = string_new();
	fseek(file, 16, SEEK_SET);
	guardar_posterior(file, &posterior);
	//printf("Posterior: %s", posterior);
	fseek(file, 17, SEEK_SET);
	int size_int = obtener_file_size(file, bloques_file);
	char* size = string_itoa(size_int);
	fputs(size, file);
	fputc('\n', file);
	fputs(posterior, file);
	fseek(file, -string_length(posterior), SEEK_END);
	free(posterior);
	free(size);
}

void actualizar_metadata(FILE* bloque_file, FILE* file, char* ultimo_bloque,
		char* bloque) {
	//Actualizar en Metadata
	char caracter;
	char* bloque_string = string_new();
	string_append(&bloque_string, ultimo_bloque);
	char* posterior = string_new();
	fseek(file, 0, SEEK_SET);
	avanzar_hasta(']', &file);
	guardar_posterior(file, &posterior);
	fseek(file, 0, SEEK_SET);
	caracter = fgetc(file);
	while (caracter != ']') {
		caracter = fgetc(file);
	}
	/*while (caracter != ',' && caracter != '[' ) {
	 fseek(file, -2, SEEK_CUR);
	 caracter = fgetc(file);
	 //printf("Char: %c\n", caracter);
	 }*/
	fseek(file, -1, SEEK_CUR);
	//printf("Resultado: %d\n", bloque);
	if (!string_equals_ignore_case(bloque, ""))
		fputc(',', file);
	string_append(&bloque_string, "]\n");
	fputs(bloque_string, file);
	fputs(posterior, file);
	free(posterior);
	free(bloque_string);
}

void verificar_validez_de_path(FILE* file){
	if(file < 0){
		log_error(logger_gamecard, "PATH INEXISTENTE\n");
		exit(1);
	}
}

void crear_nuevo_bloque(FILE* file, t_new_pokemon* new_pokemon, char* bloque) {
	//Crear nuevo bloque
	char* ultimo_bloque = obtener_bloque_disponible();
	char* bloque_path = obtener_bloque_path(ultimo_bloque);
	FILE* bloque_file = fopen(bloque_path, "w+b");
	verificar_validez_de_path(bloque_file);
	free(bloque_path);
	char* auxiliar = string_itoa(new_pokemon->pos_x);
	fputs(auxiliar, bloque_file);
	free(auxiliar);
	fputc('-', bloque_file);
	auxiliar = string_itoa(new_pokemon->pos_y);
	fputs(auxiliar, bloque_file);
	free(auxiliar);
	fputc('=', bloque_file);
	auxiliar = string_itoa(new_pokemon->cantidad);
	fputs(auxiliar, bloque_file);
	free(auxiliar);
	fputc('\n', bloque_file);

	//Actualizar en Metadata
	actualizar_metadata(bloque_file, file, ultimo_bloque, bloque);
	fclose(bloque_file);
	free(ultimo_bloque);
}

t_list* obtener_bloques_del_pokemon(FILE* file) {
	char* string_auxiliar;
	fseek(file, 0, SEEK_SET);
	avanzar_hasta('K', &file);
	avanzar_hasta('=', &file);
	char* bloques_string = guardar_hasta('\n', &file);
	t_list* bloques = list_create();
	char* bloque_leido = string_new();
	int i = 0;
	while (bloques_string[i] != '\0') {
		if (bloques_string[i] != ',' && bloques_string[i] != '['
				&& bloques_string[i] != ']')
			string_append_with_format(&bloque_leido, "%c", bloques_string[i]);
		else if (bloques_string[i] == ',' || bloques_string[i] == ']') {
			string_auxiliar = string_duplicate(bloque_leido);
			list_add(bloques, string_auxiliar);
			free(bloque_leido);
			bloque_leido = string_new();
		}

		i++;
	}

	free(bloques_string);
	free(bloque_leido);

	return bloques;
}

bool posicion_ya_cargada(char* posicion_actual, t_list* posiciones) {
	bool es_la_posicion_buscada(void* parametro) {
		char* posicion = (char*) parametro;
		return string_starts_with(posicion, posicion_actual);
	}
	return list_any_satisfy(posiciones, es_la_posicion_buscada);
}

bool no_entra_en_bloque(FILE* file, t_new_pokemon* new_pokemon,
		char* ultimo_bloque) {
	char* cadena_a_agregar = string_new();
	string_append_with_format(&cadena_a_agregar, "%s-%s=%s\n",
			string_itoa(new_pokemon->pos_x), string_itoa(new_pokemon->pos_y),
			string_itoa(new_pokemon->cantidad));
	char* bloque_path = obtener_bloque_path(ultimo_bloque);
	FILE* bloque_file = fopen(bloque_path, "r+");
	verificar_validez_de_path(bloque_file);
	int diferencia_de_sizes = (metadata_general->block_size
			- (file_size(bloque_file) + string_length(cadena_a_agregar)));
	bool entra_en_bloque = (diferencia_de_sizes >= 0);
	fclose(bloque_file);
	return !entra_en_bloque;
}

t_list* obtener_posiciones_actuales(FILE* file, t_list* bloques_file,
		t_list* bloques) {
	FILE* bloque_file;
	char* posicion;
	char* auxiliar;
	bool llego_al_final = false;
	int i = 0;
	t_list* posiciones = list_create();
	bloque_file = list_get(bloques_file, i);
	fseek(bloque_file, 1, SEEK_SET);
	while (!feof(bloque_file)) {
		fseek(bloque_file, -1, SEEK_CUR);
		posicion = guardar_hasta_con_EOF('\n', &bloque_file, &llego_al_final);
		if (llego_al_final) {
			i++;
			bloque_file = list_get(bloques_file, i);
			auxiliar = guardar_hasta('\n', &bloque_file);
			string_append_with_format(&posicion, auxiliar);
			free(auxiliar);
		}
		string_append(&posicion, "\n");
		list_add(posiciones, posicion);
		fgetc(bloque_file);
	}

	return posiciones;
}

uint32_t obtener_cantidad(char* posicion) {
	char* cantidad = string_new();
	char caracter;
	int i = 0;
	caracter = posicion[i];
	while (caracter != '=') {
		i++;
		caracter = posicion[i];
	}
	i++;
	caracter = posicion[i];
	while (caracter != '\0') {
		string_append_with_format(&cantidad, "%c", caracter);
		i++;
		caracter = posicion[i];
	}

	uint32_t cantidad_entero = atoi(cantidad);
	free(cantidad);

	return cantidad_entero;
}

t_list* obtener_bloques_actuales(FILE* file, t_list* bloques) {
	t_list* bloques_file = list_create();
	char* bloque_path;
	FILE* bloque_file;
	for (int i = 0; i < list_size(bloques); i++) {
		bloque_path = obtener_bloque_path(list_get(bloques, i));
		bloque_file = fopen(bloque_path, "r+");
		verificar_validez_de_path(bloque_file);
		list_add(bloques_file, bloque_file);
		free(bloque_path);
	}
	return bloques_file;
}

int obtener_indice_del_encontrado(int* indice_del_encontrado,
		t_list* posiciones, char* posicion_actual) {
	bool encontrado = false;
	for (int i = 0; i < list_size(posiciones) && !encontrado; i++) {
		encontrado = string_starts_with(list_get(posiciones, i),
				posicion_actual);
		if (encontrado)
			*indice_del_encontrado = i;
	}
	return *indice_del_encontrado;
}

char* obtener_posicion_actual(t_new_pokemon* new_pokemon) {
	char* posicion_actual = string_new();
	char* pos_x = string_itoa(new_pokemon->pos_x);
	char* pos_y = string_itoa(new_pokemon->pos_y);
	string_append_with_format(&posicion_actual, "%s-%s=", pos_x, pos_y);
	free(pos_x);
	free(pos_y);
	return posicion_actual;
}

char* cargar_nueva_posicion(uint32_t cantidad, char* posicion) {
	char** xey = string_split(posicion, "=");
	free(posicion);
	posicion = xey[0];
	string_append(&posicion, "=");
	string_append(&posicion, string_itoa(cantidad));
	string_append(&posicion, "\n");
	return posicion;
}

FILE* posicionar_en_inicio(t_list* bloques_file, FILE* bloque_file) {
	for (int i = 0; i < list_size(bloques_file); i++) {
		bloque_file = list_get(bloques_file, i);
		fseek(bloque_file, 0, SEEK_SET);
	}
	return bloque_file;
}

void actualizar_posiciones_ya_cargadas(t_list* posiciones, FILE* bloque_file,
		t_list* bloques_file, FILE* file, char* ultimo_bloque) {
	char* pos;
	char* bloque;
	int contador = 0;
	int k = 0;

	for (int i = 0; i < list_size(posiciones); i++) {
		pos = list_get(posiciones, i);
		for (int j = 0; j < string_length(pos); j++) {
			if (contador < (metadata_general->block_size)) {
				bloque_file = list_get(bloques_file, k);
				fputc(pos[j], bloque_file);
				contador++;
			} else if (k < (list_size(bloques_file) - 1)) {
				contador = 0;
				k++;
				bloque_file = list_get(bloques_file, k);
				fputc(pos[j], bloque_file);
			} else {
				contador = 0;
				//Crear nuevo bloque y ponerlo ah??
				bloque = obtener_bloque_disponible();
				actualizar_metadata(bloque_file, file, bloque, ultimo_bloque);
				bloque_file = fopen(obtener_bloque_path(bloque), "w+b");
				verificar_validez_de_path(bloque_file);
				free(ultimo_bloque);
				list_add(bloques_file, bloque_file);
				k++;
				fputc(pos[j], list_get(bloques_file, k));
			}
		}
	}

}

void cerrar_bloques_file(t_list* bloques_file, FILE* bloque_file) {
	for (int i = 0; i < list_size(bloques_file); i++) {
		bloque_file = list_get(bloques_file, i);
		fclose(bloque_file);
	}
}

FILE* cargar_posiciones_inexistentes(int contador, char* posicion_a_agregar,
		FILE* bloque_file, char* bloque, FILE* file, char* ultimo_bloque,
		t_list* bloques_file) {

	for (int j = 0; j < string_length(posicion_a_agregar); j++) {
		if (contador < (metadata_general->block_size)) {
			fputc(posicion_a_agregar[j], bloque_file);
			contador++;

		} else {
			contador = 0;
			//Crear nuevo bloque y ponerlo ah??
			if(bloque != NULL) free(bloque);
			bloque = obtener_bloque_disponible();
			actualizar_metadata(bloque_file, file, bloque, ultimo_bloque);
			//actualizar_size(file, bloques_file);
			//fclose(bloque_file);
			char* path_auxiliar = obtener_bloque_path(bloque);
			bloque_file = fopen(path_auxiliar, "w+b");
			verificar_validez_de_path(bloque_file);
			free(path_auxiliar);
			//free(bloque);
			list_add(bloques_file, bloque_file);
			fputc(posicion_a_agregar[j], bloque_file);
		}

	}
	if(bloque != NULL) free(bloque);
	actualizar_size(file, bloques_file);
	return bloque_file;
}

void actualizar_posiciones(FILE* file, t_new_pokemon* new_pokemon) {
	char* ultimo_bloque = obtener_ultimo_bloque(file);
	t_list* bloques;
	t_list* bloques_file;
	FILE* bloque_file;

	if (string_equals_ignore_case(ultimo_bloque, "")) {
		//Crear nuevo bloque
		crear_nuevo_bloque(file, new_pokemon, ultimo_bloque);
		bloques = obtener_bloques_del_pokemon(file);
		bloques_file = obtener_bloques_actuales(file, bloques);
		//Hasta ac?? supuestamente est?? liberado
		actualizar_size(file, bloques_file);
		cerrar_bloques_file(bloques_file, bloque_file);
		list_destroy(bloques_file);
		for(int i=0; i<list_size(bloques); i++) free(list_get(bloques,i));
		list_destroy(bloques);
		free(ultimo_bloque);
	} else {
		//Actualizar ultimo_bloque (Validar que no supere BLOCK_SIZE)
		char* bloque = NULL;
		char* posicion_actual = obtener_posicion_actual(new_pokemon);
		bloques = obtener_bloques_del_pokemon(file);
		bloques_file = obtener_bloques_actuales(file, bloques);
		t_list* posiciones = obtener_posiciones_actuales(file, bloques_file, bloques);

		if (posicion_ya_cargada(posicion_actual, posiciones)) {
			int indice_del_encontrado = 0;

			uint32_t cantidad = 0;
			obtener_indice_del_encontrado(&indice_del_encontrado, posiciones, posicion_actual);
			char* posicion = list_get(posiciones, indice_del_encontrado);

			cantidad = obtener_cantidad(posicion) + new_pokemon->cantidad;
			posicion = cargar_nueva_posicion(cantidad, posicion);
			list_replace(posiciones, indice_del_encontrado, posicion);

			posicionar_en_inicio(bloques_file, bloque_file);

			actualizar_posiciones_ya_cargadas(posiciones, bloque_file, bloques_file, file, ultimo_bloque);

			actualizar_size(file, bloques_file);

		} else {
			/*char* bloque_path = obtener_bloque_path(ultimo_bloque);
			bloque_file = fopen(bloque_path, "r+");*/
			bloque_file = list_get(bloques_file, list_size(bloques_file)-1);
			int contador = file_size(bloque_file);
			char* posicion_a_agregar = string_new();
			char* pos_x = string_itoa(new_pokemon->pos_x);
			char* pos_y = string_itoa(new_pokemon->pos_y);
			char* cantidad = string_itoa(new_pokemon->cantidad);
			string_append_with_format(&posicion_a_agregar, "%s-%s=%s\n",pos_x,pos_y,cantidad);
			free(pos_x);
			free(pos_y);
			free(cantidad);
			fseek(bloque_file, 0, SEEK_END);
			bloque_file = cargar_posiciones_inexistentes(contador,posicion_a_agregar, bloque_file, bloque, file, ultimo_bloque, bloques_file);
			free(posicion_a_agregar);
		}
		cerrar_bloques_file(bloques_file, bloque_file);
		list_destroy(bloques_file);
		for(int i=0; i<list_size(posiciones); i++) free(list_get(posiciones, i));
		list_destroy(posiciones);
		for(int i=0; i<list_size(bloques); i++) free(list_get(bloques,i));
		list_destroy(bloques);
		free(posicion_actual);
		free(ultimo_bloque);
	}

}

char* generar_nombre(char* parametro) {
	char* nombre = string_new();
	string_append(&nombre, config_gamecard->punto_montaje_tallgrass);
	string_append(&nombre, parametro);
	return nombre;
}

char* generar_pokemon_metadata_bin_path(char* pokemon) {
	char* path = generar_pokemon_file_path(pokemon);
	string_append(&path, "/Metadata.bin");
	return path;
}

void generar_metadata_bin(char* pokemon) {
	char* metadata_bin_path = generar_pokemon_metadata_bin_path(pokemon);
	FILE* metadata_file = fopen(metadata_bin_path, "w+b");
	verificar_validez_de_path(metadata_file);
	fputs("DIRECTORY=N\n", metadata_file);
	fputs("SIZE=0\n", metadata_file);
	fputs("BLOCKS=[]\n", metadata_file);
	fputs("OPEN=N", metadata_file);
	fclose(metadata_file);
	free(metadata_bin_path);
}
