
target("misaka_sisters")
    set_kind("binary")

    -- std=c++14
    set_languages("c99", "cxx14")

    -- jsoncpp
    add_includedirs("src/jsoncpp")

    -- source files
    add_files("src/*.cpp")
    add_files("src/jsoncpp/*.cpp")

    -- link flags
    add_links("pthread", "curl", "mysqlclient")

    -- build dir
    set_targetdir(".")
    set_objectdir("build/objs")

    add_mflags("-O2")
