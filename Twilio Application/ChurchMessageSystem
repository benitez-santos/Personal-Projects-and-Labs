# Download the helper library from https://www.twilio.com/docs/python/install
from twilio.rest import Client
from phoneInformation import account_sid, auth_token, my_twilio_phoneNumber
import csv 
sizeOfCSV = 0
with open('ChurchNamesAndPhoneNumbers.csv') as csvfile:
    namesAndPhoneNums = csv.reader(csvfile, delimiter=',')
    sizeOfCSV = sum(1 for row in namesAndPhoneNums)
    print(str(sizeOfCSV))
phoneNumbers = ["" for x in range(sizeOfCSV)]
counter = 0
with open('ChurchNamesAndPhoneNumbers.csv') as csvfile:
    namesAndPhoneNums = csv.reader(csvfile, delimiter=',')
    for row in namesAndPhoneNums:
        print(row[1])
        phoneNumbers[counter] = '+1'+row[1]
        counter = counter +1

print(phoneNumbers)

client = Client(account_sid, auth_token)
message_to_send = 'Paz de Cristo, estudio hoy a las 7PM. Link para Zoom: https://us02web.zoom.us/j/84404545074?pwd=TndGQnRRL0dOdi90VSV0RLbUcxZz09'
for index in range(len(phoneNumbers)):
    message = client.messages.create(
                                  body = message_to_send,
                                  from_ = my_twilio_phoneNumber,
                                  to = phoneNumbers[index]
                              )

print(message.sid)

