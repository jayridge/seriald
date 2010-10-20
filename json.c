#include "seriald.h"

void json_to_bson_append_array( bson_buffer * bb , struct json_object * a );
void json_to_bson_append( bson_buffer * bb , struct json_object * o );


void json_to_bson_append_element( bson_buffer * bb , const char * k , struct json_object * v ){
    if ( ! v ){
        bson_append_null( bb , k );
        return;
    }
    
    switch ( json_object_get_type( v ) ){
    case json_type_int:
        bson_append_int( bb , k , json_object_get_int( v ) );
        break;
    case json_type_boolean:
        bson_append_bool( bb , k , json_object_get_boolean( v ) );
        break;
    case json_type_double:
        bson_append_double( bb , k , json_object_get_double( v ) );
        break;
    case json_type_string:
        bson_append_string( bb , k , json_object_get_string( v ) );
        break;
    case json_type_object:
        bson_append_start_object( bb , k );
        json_to_bson_append( bb , v );
        bson_append_finish_object( bb );
        break;
    case json_type_array:
        bson_append_start_array( bb , k );
        json_to_bson_append_array( bb , v );
        bson_append_finish_object( bb );
        break;
    default:
        fprintf( stderr , "can't handle type for : %s\n" , json_object_to_json_string(v) );
    }
}

/**
   should already have called start_array
   this will not call start/finish
 */
void json_to_bson_append_array( bson_buffer * bb , struct json_object * a ){
    int i;
    char buf[10];
    for ( i=0; i<json_object_array_length( a ); i++){
        sprintf( buf , "%d" , i );
        json_to_bson_append_element( bb , buf , json_object_array_get_idx( a , i ) );
    }
}

void json_to_bson_append( bson_buffer * bb , struct json_object * o ){
    json_object_object_foreach(o,k,v){
        json_to_bson_append_element( bb , k , v );
    }
}

char * json_to_bson( char * js, int *size ){
    struct json_object * o = json_tokener_parse(js);
    bson_buffer bb;
    char *data;
    
    if ( is_error( o ) ){
        fprintf( stderr , "\t ERROR PARSING\n" );
        return 0;
    }
    
    if ( ! json_object_is_type( o , json_type_object ) ){
        fprintf( stderr , "json_to_bson needs a JSON object, not type\n" );
        return 0;
    }
    
    bson_buffer_init( &bb );
    json_to_bson_append( &bb , o );
    data = bson_buffer_finish( &bb );
    *size = bb.bufSize;
    return data;
}

