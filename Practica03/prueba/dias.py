import datetime

def dia_actual():
    return datetime.datetime.now().day

def dia_siguiente():
    return datetime.datetime.now().day + 1

if __name__ == '__main__':
    print(dia_actual())
    print(dia_siguiente())