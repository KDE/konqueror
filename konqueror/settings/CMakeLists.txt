
add_subdirectory( ebrowsing ) 
add_subdirectory( konq ) 
# k3process
if(NOT WIN32)
  add_subdirectory( konqhtml ) 
endif(NOT WIN32)
add_subdirectory( performance ) 
add_subdirectory( kio ) 
add_subdirectory( filetypes ) 
add_subdirectory( pics ) 
add_subdirectory( css ) 

