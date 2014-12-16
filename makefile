
default:
	cd src/Point && make -k all VERSION=RELEASE

clean:
	cd src/Point && make -k cleanall VERSION=RELEASE

debug:
	cd src/Point && make -k all VERSION=DEBUG

clean.debug:
	cd src/Point && make -k cleanall VERSION=DEBUG
