CFLAGS=-I/usr/include/libxml2

COLL_OBJ = glb_coll_server.o\
	  glb_collection.o

STORAGE_OBJ = glb_storage_server.o\
          glb_storage.o\
          glb_partition.o\
	  glb_set.o\
	  glb_set_file.o\
	  glb_index_tree.o\
	  glb_utils.o\
	  glb_request_handler.o\
	  glb_interpreter.o\
	  glb_maze.o\
	  oodict.o\
	  ooarray.o\
	  oolist.o

TEST_CLIENT_OBJ = glb_test_client.o

program: $(COLL_OBJ) $(STORAGE_OBJ) $(TEST_CLIENT_OBJ) 
	cc -o glb_coll_server $(COLL_OBJ) -lzmq -lpthread -Wall -pedantic -Ansi
	cc -o glb_storage_server $(STORAGE_OBJ) -lzmq -lpthread -lxml2 -Wall -pedantic -Ansi
	cc -o glb_test_client $(TEST_CLIENT_OBJ) -lzmq -Wall -pedantic -Ansi


clean:
	rm -rf *.o
