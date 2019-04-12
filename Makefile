all: kap_ssd_lab_2

kap_ssd_lab_2: src/main.cpp src/photo_rest_service.cpp src/flickr_requester.cpp
	g++ $^ -lcpprest -lpthread -lssl -lcrypto -lboost_system -o bin/$@

kap_ssd_lab_2_static: src/main.cpp
	g++ $^ \
		-o bin/$@ -static \
		-L/home/k-knight/projects/cpp/KAP_SSD_LAB_2/libs/cpprest/libcpprest.so \
		-L/home/k-knight/projects/cpp/KAP_SSD_LAB_2/libs/openssl/libcrypto.so \
		-L/home/k-knight/projects/cpp/KAP_SSD_LAB_2/libs/openssl/linssl.so \
		-lpthread -lboost_system 