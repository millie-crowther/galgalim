FROM python
COPY ./requirements.txt /
RUN pip install -r /requirements.txt

COPY ./app.py /
ENV FLASK_DEBUG=1
CMD ["flask", "run", "--host=0.0.0.0", "--port=80", "--cert=adhoc"]
