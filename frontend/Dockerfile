FROM python
COPY ./requirements.txt /
RUN pip install -r /requirements.txt

COPY ./app.py /
CMD ["flask", "run"]
