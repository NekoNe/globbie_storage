OBJ = glb_server.o\
	  glb_set.o\
	  glb_set_file.o\
	  glb_index_tree.o\
	  glb_utils.o\
	  glb_request_handler.o\
	  glb_interpreter.o\
	  glb_collection.o\
	  glb_maze.o\
	  oodict.o\
	  ooarray.o\
	  oolist.o
	  
	 

program: $(OBJ)
	cc -o glb_server $(OBJ) -lzmq -lpthread -Wall -pedantic -Ansi

clean:
	rm -rf *.o
