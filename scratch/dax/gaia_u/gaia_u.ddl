create table if not exists Persons 
(
      FirstName : string,
      LastName : string,
      BirthDate : int64,
      FaceSignature : string
);

create table if not exists Buildings 
(
      BuildingName : string,
      DoorLocked : bool active
);

create table if not exists Staff 
(
      HiredDate : int64,
      references Persons
);

create table if not exists Parents 
(
      FatherOrMother : string,
      references Persons
);

create table if not exists Students 
(
      Number : uint32,
      references Persons
);

create table if not exists Family 
(
      references Students,
      Father references Parents,
      Mother references Parents
);

create table if not exists FaceScanLog 
(
      ScanSignature : string,
      ScanDate : int64,
      ScanTime : int64,
      references Buildings
);

create table if not exists Rooms 
(
      RoomNumber : uint16,
      RoomName : string,
      FloorNumber : uint8,
      Capacity : uint16,
      references Buildings
);

create table if not exists Events 
(
      Name : string,
      Date : int64,
      StartTime : int64,
      EndTime : int64,
      Enrolled : uint32,
      Teacher references Staff,
      Room references Rooms
);

create table if not exists Registration 
(
      RegistrationDate : int64,
      RegistrationTime : int64,
      Event references Events,
      Student references Students
);
