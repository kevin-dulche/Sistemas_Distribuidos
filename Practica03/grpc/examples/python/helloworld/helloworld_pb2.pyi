from google.protobuf import empty_pb2 as _empty_pb2
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Mapping as _Mapping, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class HelloRequest(_message.Message):
    __slots__ = ("name",)
    NAME_FIELD_NUMBER: _ClassVar[int]
    name: str
    def __init__(self, name: _Optional[str] = ...) -> None: ...

class HelloReply(_message.Message):
    __slots__ = ("message",)
    MESSAGE_FIELD_NUMBER: _ClassVar[int]
    message: str
    def __init__(self, message: _Optional[str] = ...) -> None: ...

class Date(_message.Message):
    __slots__ = ("day", "month", "year")
    DAY_FIELD_NUMBER: _ClassVar[int]
    MONTH_FIELD_NUMBER: _ClassVar[int]
    YEAR_FIELD_NUMBER: _ClassVar[int]
    day: int
    month: int
    year: int
    def __init__(self, day: _Optional[int] = ..., month: _Optional[int] = ..., year: _Optional[int] = ...) -> None: ...

class DateReply(_message.Message):
    __slots__ = ("day", "month", "year")
    DAY_FIELD_NUMBER: _ClassVar[int]
    MONTH_FIELD_NUMBER: _ClassVar[int]
    YEAR_FIELD_NUMBER: _ClassVar[int]
    day: int
    month: int
    year: int
    def __init__(self, day: _Optional[int] = ..., month: _Optional[int] = ..., year: _Optional[int] = ...) -> None: ...

class DateInt(_message.Message):
    __slots__ = ("date", "days")
    DATE_FIELD_NUMBER: _ClassVar[int]
    DAYS_FIELD_NUMBER: _ClassVar[int]
    date: Date
    days: int
    def __init__(self, date: _Optional[_Union[Date, _Mapping]] = ..., days: _Optional[int] = ...) -> None: ...

class Distancia(_message.Message):
    __slots__ = ("distancia",)
    DISTANCIA_FIELD_NUMBER: _ClassVar[int]
    distancia: float
    def __init__(self, distancia: _Optional[float] = ...) -> None: ...

class DisparaCannonArgs(_message.Message):
    __slots__ = ("nombreUsuario", "anguloRadianes", "velocidadMtrsSeg")
    NOMBREUSUARIO_FIELD_NUMBER: _ClassVar[int]
    ANGULORADIANES_FIELD_NUMBER: _ClassVar[int]
    VELOCIDADMTRSSEG_FIELD_NUMBER: _ClassVar[int]
    nombreUsuario: str
    anguloRadianes: int
    velocidadMtrsSeg: float
    def __init__(self, nombreUsuario: _Optional[str] = ..., anguloRadianes: _Optional[int] = ..., velocidadMtrsSeg: _Optional[float] = ...) -> None: ...

class DisparaCannonReturn(_message.Message):
    __slots__ = ("distancia",)
    DISTANCIA_FIELD_NUMBER: _ClassVar[int]
    distancia: float
    def __init__(self, distancia: _Optional[float] = ...) -> None: ...

class mejorDisparoReturn(_message.Message):
    __slots__ = ("nombreMejorDisparo", "distancia")
    NOMBREMEJORDISPARO_FIELD_NUMBER: _ClassVar[int]
    DISTANCIA_FIELD_NUMBER: _ClassVar[int]
    nombreMejorDisparo: str
    distancia: float
    def __init__(self, nombreMejorDisparo: _Optional[str] = ..., distancia: _Optional[float] = ...) -> None: ...
