#include "seriald.h"


void srld_iterator_init( srld_db *db, srld_iterator *iterator )
{
    memset(iterator, 0, sizeof(*iterator));
    iterator->db = db;
    gettimeofday(iterator->start, NULL);
}


srld_record * srld_first( srld_iterator *iterator )
{
}


srld_record * srld_last( srld_iterator *iterator )
{
}


srld_record * srld_next( srld_iterator *iterator )
{

}


srld_record * srld_previous( srld_iterator *iterator )
{
}


off_t srld_tell( srld_iterator *iterator )
{
}
