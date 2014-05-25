add_executable(service_example 
			   service_example.cpp)

target_link_libraries(service_example
					  pthread)
