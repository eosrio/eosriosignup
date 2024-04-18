project(eosriosignup)

find_package(cdt)

add_contract( eosriosignup eosriosignup eosriosignup.cpp )
target_include_directories( eosriosignup PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( eosriosignup ${CMAKE_SOURCE_DIR}/../ricardian )