/*
 * MessagePack for C memory pool implementation
 *
 * Copyright (C) 2008-2009 FURUHASHI Sadayuki
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#include "msgpack/zone.h"
#include <stdlib.h>
#include <string.h>

struct msgpack_zone_chunk {
	struct msgpack_zone_chunk* next;
	/* data ... */
};

static int init_chunk_list(msgpack_zone_chunk_list* cl, size_t chunk_size)
{
	msgpack_zone_chunk* chunk = (msgpack_zone_chunk*)malloc(
			sizeof(msgpack_zone_chunk) + chunk_size);
	if(chunk == NULL) {
		return 0;
	}

	cl->head = chunk;
	cl->free = chunk_size;
	cl->ptr  = ((char*)chunk) + sizeof(msgpack_zone_chunk);
	chunk->next = NULL;

	return 1;
}

static void destroy_chunk_list(msgpack_zone_chunk_list* cl)
{
	msgpack_zone_chunk* c = cl->head;
	while(1) {
		msgpack_zone_chunk* n = c->next;
		free(c);
		if(n != NULL) {
			c = n;
		} else {
			break;
		}
	}
}

static void clear_chunk_list(msgpack_zone_chunk_list* cl, size_t chunk_size)
{
	msgpack_zone_chunk* c = cl->head;
	while(1) {
		msgpack_zone_chunk* n = c->next;
		if(n != NULL) {
			free(c);
			c = n;
		} else {
			break;
		}
	}
	cl->head->next = NULL;
	cl->free = chunk_size;
	cl->ptr  = ((char*)cl->head) + sizeof(msgpack_zone_chunk);
}

void* msgpack_zone_malloc_expand(msgpack_zone* zone, size_t size)
{
	msgpack_zone_chunk_list* const cl = &zone->chunk_list;
	size_t sz = zone->chunk_size;
	msgpack_zone_chunk* chunk;
	char* ptr;

	while(sz < size) {
		sz *= 2;
	}

	chunk = (msgpack_zone_chunk*)malloc(sizeof(msgpack_zone_chunk) + sz);

	ptr = ((char*)chunk) + sizeof(msgpack_zone_chunk);

	chunk->next = cl->head;
	cl->head = chunk;
	cl->free = sz - size;
	cl->ptr  = ptr + size;

	return ptr;
}


static void init_finalizer_array(msgpack_zone_finalizer_array* fa)
{
	fa->tail  = NULL;
	fa->end   = NULL;
	fa->array = NULL;
}

static void call_finalizer_array(msgpack_zone_finalizer_array* fa)
{
	msgpack_zone_finalizer* fin = fa->tail;
	for(; fin != fa->array; --fin) {
		(*(fin-1)->func)((fin-1)->data);
	}
}

static void destroy_finalizer_array(msgpack_zone_finalizer_array* fa)
{
	call_finalizer_array(fa);
	free(fa->array);
}

static void clear_finalizer_array(msgpack_zone_finalizer_array* fa)
{
	call_finalizer_array(fa);
	fa->tail = fa->array;
}

int msgpack_zone_push_finalizer_expand(msgpack_zone* zone,
		void (*func)(void* data), void* data)
{
	msgpack_zone_finalizer_array* const fa = &zone->finalizer_array;
	const size_t nused = fa->end - fa->array;
	msgpack_zone_finalizer* tmp;

	size_t nnext;
	if(nused == 0) {
		nnext = (sizeof(msgpack_zone_finalizer) < 72/2) ?
				72 / sizeof(msgpack_zone_finalizer) : 8;

	} else {
		nnext = nused * 2;
	}

	tmp = (msgpack_zone_finalizer*)realloc(fa->array,
			sizeof(msgpack_zone_finalizer) * nnext);
	if(tmp == NULL) {
		return 0;
	}

	fa->array  = tmp;
	fa->end    = tmp + nnext;
	fa->tail   = tmp + nused;

	fa->tail->func = func;
	fa->tail->data = data;

	++fa->tail;

	return 1;
}


int msgpack_zone_is_empty(msgpack_zone* zone)
{
	msgpack_zone_chunk_list* const cl = &zone->chunk_list;
	msgpack_zone_finalizer_array* const fa = &zone->finalizer_array;
	return cl->free == zone->chunk_size && cl->head->next == NULL &&
		fa->tail == fa->array;
}


void msgpack_zone_destroy(msgpack_zone* zone)
{
	destroy_finalizer_array(&zone->finalizer_array);
	destroy_chunk_list(&zone->chunk_list);
}

void msgpack_zone_clear(msgpack_zone* zone)
{
	clear_finalizer_array(&zone->finalizer_array);
	clear_chunk_list(&zone->chunk_list, zone->chunk_size);
}

int msgpack_zone_init(msgpack_zone* zone, size_t chunk_size)
{
	zone->chunk_size = chunk_size;

	if(!init_chunk_list(&zone->chunk_list, chunk_size)) {
		return 0;
	}

	init_finalizer_array(&zone->finalizer_array);

	return 1;
}

msgpack_zone* msgpack_zone_new(size_t chunk_size)
{
	msgpack_zone* zone = (msgpack_zone*)malloc(
			sizeof(msgpack_zone) + chunk_size);
	if(zone == NULL) {
		return NULL;
	}

	zone->chunk_size = chunk_size;

	if(!init_chunk_list(&zone->chunk_list, chunk_size)) {
		free(zone);
		return NULL;
	}

	init_finalizer_array(&zone->finalizer_array);

	return zone;
}

void msgpack_zone_free(msgpack_zone* zone)
{
	if(zone == NULL) { return; }
	msgpack_zone_destroy(zone);
	free(zone);
}

