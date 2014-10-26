PYTHON=/usr/bin/python3
REGRESSION_TESTS=issue26.py issue31.py issue45.py issue46.py \
			issue50.py issue51.py issue55.py issue58.py \
			issue72.py \
			filehash.py

regress:
	@for test in $(REGRESSION_TESTS); do \
		echo $$test; \
		$(PYTHON) $$test; \
	done

clean:
	rm -f *.pyc
