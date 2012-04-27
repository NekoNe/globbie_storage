OBJ = glb_server.o\
	  glb_set.o\
	  glb_set_file.o\
	  glb_index_tree.o\
	  glb_utils.o\
	  glb_request_handler.o\
	  glb_collection.o\
	  oodict.o\
	  ooarray.o\
	  oolist.o
	  
	 

program: $(OBJ)
	cc -o program $(OBJ) -lzmq -lpthread -Wall -pedantic -Ansi

clean:
	rm -rf *.o
