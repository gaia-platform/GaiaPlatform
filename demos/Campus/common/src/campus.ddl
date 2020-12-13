create table if not exists campus (
      name : string,
      in_emergency : bool active
);

create table if not exists building (
      name : string,
      BuildingCampus references campus
);

create table if not exists locations (
      name : string,
      LocationsCampus references campus
);

create table if not exists Persons (
      PersonId : string,      
      FirstName : string,      
      LastName : string,      
      BirthDate : string,      
      FaceSignature : string,
      PersonsCampus references campus
);

create table if not exists person (
      name : string,
      is_threat : uint64 active,
      location : string active,
      PersonCampus references campus
);

create table if not exists role (
      name : string,
      RolePerson references person
);

create table if not exists Students (
      StudentId : string,        
      Father : string,        
      Mother : string,      
      Number : uint64,
      StudentsPersons references Persons
);

create table if not exists Staff (
      StaffId : string active,     
      HiredDate : string,
      StaffPersons references Persons
);

create table if not exists Parents (
      ParentId : string,        
      ForM : bool,        
      ParentsPersons references Persons
);

create table if not exists Family (
      FamilyID : string, 
      FamilyStudents references Students,
      FamilyParents references Parents
);

create table if not exists Buildings (
      BuildingId : string, 
      BuildingName : string,
      BuildingsCampus references campus
);

create table if not exists Rooms (
      RoomNumber: string,      
      RoomName: string,      
      FloorNumber: string,        
      Capacity: uint16,
      RoomsBuildings references Buildings
);

create table if not exists Events (
      EventId : string,
      Name : string, 
      EventDate : string,         
      StartTime : string,         
      EndTime : string,       
      Enrolled : uint16,
      EventsCampus references campus,
      EventsRooms references Rooms,
      EventsStaff references Staff
);

create table if not exists Registration (
      RegistrationID : string,    
      RegistrationDate : string,         
      RegistrationTime : string,   
      RegistrationEvents references Events,
      RegistrationPerson references person
);


create table if not exists Cameras (
      CameraId : string, 
      DoorClosed bool,          
      CamerasRooms references Rooms
);

create table if not exists Strangers (
      StrangerId : string, 
      FirstScanned : string, 
      ScanCount uint64,
      StrangersPersons references Persons
);

create table if not exists FaceScanLog (
      FaceScanLogId : string, 
      ScanSignature : string, 
      ScanSignatureId: string,      
      Building: string,      
      ScanDate: string,           
      ScanTime: string,        
      FaceScanLogPersons references Persons,      
      FaceScanLogStrangers references Strangers
);