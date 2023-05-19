CXX ?= g++

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Configure compile flags.
CXXFLAGS ?= -ggdb3 -std=c++17 -Wall -Wextra -Weffc++ 				   \
			-Wc++14-compat 				   \
			-Wmissing-declarations -Wcast-align -Wcast-qual 			   \
			-Wchar-subscripts \
			-Wconversion -Wctor-dtor-privacy -Wempty-body 				   \
			-Wfloat-equal -Wformat-nonliteral -Wformat-security 		   \
			-Wformat=2 -Winline \
			-Wnon-virtual-dtor -Woverloaded-virtual 		   \
			-Wpacked -Wpointer-arith -Winit-self -Wredundant-decls		   \
			-Wshadow -Wsign-conversion -Wsign-promo \
			-Wstrict-overflow=2 \
			\
			-Wsuggest-override -Wswitch-default -Wswitch-enum \
			-Wundef -Wunreachable-code -Wunused \
			-Wvariadic-macros \
			-Wno-missing-field-initializers -Wno-narrowing 				   \
			-Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new \
			-fsized-deallocation -fstack-protector -fstrict-overflow 	   \
			-fno-omit-frame-pointer -fPIE 	   \

all: main.cpp ./language/Analyzer/WriteIntoDb.cpp
	$(CXX) $(CXXFLAGS) main.cpp ./language/Analyzer/WriteIntoDb.cpp ./language/Analyzer/utils.cpp ./language/utils/src/ErrorHandlerLib.cpp ./language/utils/src/consoleColorLib.cpp ./language/Analyzer/tokenizer.cpp ./BinaryTranslator.cpp translator.cpp ./language/readerLib/functions.cpp ./language/logs/LogLib.cpp ./elfFileGen.cpp -o binTranslate




