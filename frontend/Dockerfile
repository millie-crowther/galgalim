FROM python
COPY ./requirements.txt /
RUN pip install -r /requirements.txt

COPY ./wsgi.py /
COPY ./galgalim.py /
ENV FLASK_DEBUG=1
CMD ["gunicorn", "--bind", "0.0.0.0:5000", "wsgi:app"]
