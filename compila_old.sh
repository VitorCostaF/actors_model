gcc -g -Wall -o actor_old actor.c ring_buffer.c -lpthread -D OLD_IMPL -D OUT_FILE='"output_file_old.txt"'
