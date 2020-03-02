/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>
#include <cstdint>
#include <list>
#include <map>
#include "AirportTypes.h"
#include "NullableString.h"
#include "airport_generated.h" // include both flatbuffer types and object API for testing 
#include "PerfTimer.h"

using namespace std;
using namespace gaia_se;
using namespace AirportDemo;

//
// API outside of object instead of a base class
// might be attractive because it's more easily translatable
// to C
//
namespace gaia {
    template<typename T_fb, gaia_type_t T>
    const T_fb * GetFirst() 
    {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T);
        if (node_ptr) {
            return flatbuffers::GetRoot<airports>(node_ptr->payload);
        }
        return nullptr;
    }

    template<typename T_fb>
    const T_fb * GetNext(T_fb * current) 
    {
        auto current_ptr = gaia_se_node::open(current->Gaia_id());
        auto next_ptr = current_ptr.find_next();
        if (next_ptr) {
            return flatbuffers::GetRoot<airports>(next_ptr->payload);
        }
        return nullptr;
    }

    template<typename T_fb, typename T_updateable>
    T_updateable * GetUpdatableObject(const T_fb * fb) {
        return fb->UnPack();
    }

    template<typename T_fb, typename T_updateable>
    void Update(T_updateable * obj)
    {
        flatbuffers::FlatBufferBuilder fbb;
        auto u = T_fb::Pack(fbb, obj);
        fbb.Finish(u);
        auto node_ptr = gaia_se_node::open(obj->Gaia_id);
        node_ptr.update_payload(fbb.GetSize(), fbb.GetBufferPointer());
    }


    //
    // or we could have a non-templated approach where we generate
    // the gaia syntax for each flatbuffer type that is compiled
    //
    const airports * get_first_airport() 
    {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);
        if (node_ptr) {
            return flatbuffers::GetRoot<airports>(node_ptr->payload);
        }
        return nullptr;
    }

    const airports * get_next_airport(const airports * current) 
    {
        auto current_ptr = gaia_se_node::open(current->Gaia_id());
        auto next_ptr = current_ptr.find_next();
        if (next_ptr) {
            return flatbuffers::GetRoot<airports>(next_ptr->payload);
        }
        return nullptr;
    }

    airportsT * get_updateable_airport(const airports * ap) {
        return ap->UnPack();
    }

    void update_airport(const airportsT * ap)
    {
        flatbuffers::FlatBufferBuilder fbb;
        auto u = airports::Pack(fbb, ap);
        fbb.Finish(u);
        auto node_ptr = gaia_se_node::open(ap->Gaia_id);
        node_ptr.update_payload(fbb.GetSize(), fbb.GetBufferPointer());
    }
}


// **************************************************************************
// shows augmenting the flatbuffer object class (implies copy) with
// helpers to make the syntax nicer
// **************************************************************************
struct Airport_Mutable : public airportsT {
    static Airport_Mutable * GetFirst() 
    {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);
        if (node_ptr) {
            auto row = flatbuffers::GetRoot<airports>(node_ptr->payload);
            Airport_Mutable * obj = new Airport_Mutable();
            row->UnPackTo(obj);
            return obj;
        }
        return nullptr;
    }

     static Airport_Mutable * GetFirst(Airport_Mutable * obj) {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);
        if (node_ptr) {
            auto row = flatbuffers::GetRoot<airports>(node_ptr->payload);
            row->UnPackTo(obj);
            return obj;
        }
        return nullptr;
    }


    Airport_Mutable * GetNext() {
        auto current_ptr = gaia_se_node::open(this->Gaia_id);
        auto next_ptr = current_ptr.find_next();
        if (next_ptr) {
            auto row = flatbuffers::GetRoot<airports>(next_ptr->payload);
            Airport_Mutable * obj = new Airport_Mutable();
            row->UnPackTo(obj);
            return obj;
        }
        return nullptr;
    }

    // allow client to send in pointer (instead of always allocating)
    Airport_Mutable * GetNext(Airport_Mutable * ap){
        if (ap) {
            auto current_ptr = gaia_se_node::open(this->Gaia_id);
            auto next_ptr = current_ptr.find_next();
            if (next_ptr) {
                auto row = flatbuffers::GetRoot<airports>(next_ptr->payload);
                row->UnPackTo(ap);
                return ap;
            }
        }
        
        return nullptr;
    }

    void Update()
    {
        auto u = airports::Pack(_fbb, this);
        _fbb.Finish(u);
        auto node_ptr = gaia_se_node::open(this->Gaia_id);
        node_ptr.update_payload(_fbb.GetSize(), _fbb.GetBufferPointer());
        _fbb.Clear();
    }

private:
    // for updating
    flatbuffers::FlatBufferBuilder _fbb;
};

struct Airport_ZeroCopy
{
    Airport_ZeroCopy() : _fb(nullptr) {}
    Airport_ZeroCopy(const airports * fb) : _fb(fb) {}

    static Airport_ZeroCopy * GetFirst(Airport_ZeroCopy * obj) {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);
        if (node_ptr) {
            obj->set_fb(flatbuffers::GetRoot<airports>(node_ptr->payload));
            return obj;
        }
        return nullptr;
    }

    // allow client to send in pointer (instead of always allocating)
    Airport_ZeroCopy * GetNext(Airport_ZeroCopy * ap){
        if (ap) {
            auto current_ptr = gaia_se_node::open(this->Gaia_id());
            auto next_ptr = current_ptr.find_next();
            if (next_ptr) {
                ap->set_fb(flatbuffers::GetRoot<airports>(next_ptr->payload));
                return ap;
            }
        }
        
        return nullptr;
    }

    // allow reuse of this class with a flat buffer
    void set_fb(const airports * fb)
    {
        _fb = fb;
    }

    // could be generated
    int64_t Gaia_id() const {
        return _fb->Gaia_id();
    }
    int32_t ap_id() const {
        return _fb->ap_id();
    }
    const char * name() const {
        return _fb->name() ? _fb->name()->c_str() : nullptr;
    }
    const char * city() const {
        return _fb->city() ?_fb->city()->c_str() : nullptr;
    }
    const char * country() const {
        return _fb->country() ?_fb->country()->c_str() : nullptr;
    }
    const char * iata() const {
        return _fb->iata() ?_fb->iata()->c_str() : nullptr;
    }
    const char * icao() const {
        return _fb->icao()? _fb->icao()->c_str() : nullptr;
    }
    double latitude() const {
        return _fb->latitude();
    }
    double longitude() const {
        return _fb->longitude();
    }
    int32_t altitude() const {
        return _fb->altitude();
    }
    float timezone() const {
        return _fb->timezone();
    }
    const char * dst() const {
        return _fb->dst() ? _fb->dst()->c_str() : nullptr;
    }
    const char * tztext() const {
        return _fb->tztext() ? _fb->tztext()->c_str() : nullptr;
    }
    const char * type() const {
        return _fb->type() ? _fb->type()->c_str() : nullptr;
    }
    const char * source() const {
        return _fb->source() ? _fb->source()->c_str() : nullptr;
    }        
private:
    // read-only flatbuffer
    const airports * _fb;
}; // Airport_ZeroCopy


struct GaiaBase
{
    static map<int64_t, GaiaBase *> s_gaia_cache;
    virtual ~GaiaBase() = default;
};
map<int64_t, GaiaBase *> GaiaBase::s_gaia_cache;

/*
template <gaia_se::gaia_type_t T_gaia_type, typename T_gaia, typename T_reader, typename I_reader, typename T_writer, typename T_fb>
struct GaiaRW : GaiaBase
{
public:
    virtual int64_t Gaia_id() const = 0;
    virtual ~GaiaRW() {
        reset();
    }

    GaiaRW() : _writer(nullptr), _reader(nullptr), _fbb(nullptr) {}

    static T_gaia * GetFirst() {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        return GetObject(node_ptr);
    }

    // allow client to send in pointer (instead of always allocating)
    T_gaia * GetNext() {
        auto current_ptr = gaia_se_node::open(this->Gaia_id());
        auto next_ptr = current_ptr.find_next();
        return GetObject(next_ptr);
    }

    void Update()
    {
        if (_writer) {
            // assert _fbb
            auto u = T_fb::Pack(*_fbb, _writer);
            _fbb->Finish(u);
            auto node_ptr = gaia_se_node::open(this->Gaia_id());
            node_ptr.update_payload(_fbb->GetSize(), _fbb->GetBufferPointer());
            _fbb->Clear();
        }
    } 

    // used by Airport_RW
    T_writer * writer() 
    {
        if (_writer == nullptr) {
            T_writer * writer = new T_writer;
            _fb_reader.get_fb()->UnPackTo(writer);
            _writer = writer;
            _fbb = new flatbuffers::FlatBufferBuilder();
            // redirect our reader pointer to the writer now that
            // we've made a copy
            _reader = _writer;
        }
        return _writer;
    }


protected:
    I_reader * _reader;
    T_writer * _writer;

private:
    T_reader _fb_reader;
    flatbuffers::FlatBufferBuilder * _fbb;    // cached flat buffer builder for reuse

    void set_fb(const T_fb * fb)
    {
        reset();
        _fb_reader.set_fb(fb);
    }

    static T_gaia * GetObject(gaia_ptr<gaia_se_node>& node_ptr) 
    {
        T_gaia * obj = nullptr;
        if (node_ptr) {
            auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
            auto it = s_gaia_cache.find(node_ptr->id);
            if (it != s_gaia_cache.end()){
                obj = dynamic_cast<T_gaia *>(it->second);
            }
            else {
                obj = new T_gaia();
                s_gaia_cache.insert(pair<gaia_id_t, GaiaBase *>(node_ptr->id, obj));
            }
            obj->set_fb(fb);
        }
        return obj;
    }
    void reset()
    {
        if (_writer){
            delete _writer;
        }
        if (_fbb) {
            delete _fbb;
        }
        _writer = nullptr;
        _fbb = nullptr;
        _reader = &_fb_reader;
    }

};
*/

// think about a non-template solution
template <gaia_se::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct GaiaObj : GaiaBase
{
public:
    virtual int64_t Gaia_id() const = 0;

    virtual ~GaiaObj() {
        reset();
    }

    GaiaObj() : _copy(nullptr), _fb(nullptr), _fbb(nullptr) {}

    static T_gaia * GetFirst() {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        return GetObject(node_ptr);
    }

    static T_gaia * GetFirst(T_gaia * obj) {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        if (node_ptr) {
            auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
            obj->set_fb(fb);
            return obj;
        }
        return nullptr;
    }

    // allow client to send in pointer (instead of always allocating)
    T_gaia * GetNext() {
        auto current_ptr = gaia_se_node::open(this->Gaia_id());
        auto next_ptr = current_ptr.find_next();
        return GetObject(next_ptr);
    }

    T_gaia * GetNext(T_gaia * obj) {
        auto current_ptr = gaia_se_node::open(this->Gaia_id());
        auto next_ptr = current_ptr.find_next();
        if (next_ptr) {
            auto fb = flatbuffers::GetRoot<T_fb>(next_ptr->payload);
            obj->set_fb(fb);
            return obj;
        }
        return nullptr;
    }

    void Update()
    {
        if (_copy) {
            // assert _fbb
            auto u = T_fb::Pack(*_fbb, _copy);
            _fbb->Finish(u);
            auto node_ptr = gaia_se_node::open(this->Gaia_id());
            node_ptr.update_payload(_fbb->GetSize(), _fbb->GetBufferPointer());
            _fbb->Clear();
        }
    } 

    // allow reuse of this class with a flat buffer
    void set_fb(const T_fb * fb)
    {
        reset();
        _fb = fb;
    }

    // used by Airport_ZeroCopy_Mutable
    T_obj * copy_write() {
        if (_copy == nullptr) {
            T_obj * copy = new T_obj();
            _fb->UnPackTo(copy);
            _copy = copy;
            _fbb = new flatbuffers::FlatBufferBuilder();
        }
        return _copy;
    }

protected:
    const T_fb * _fb; // zero copy flat buffer
    T_obj * _copy; // copy data changes
    flatbuffers::FlatBufferBuilder * _fbb;    // cached flat buffer builder for reuse

private:
    static T_gaia * GetObject(gaia_ptr<gaia_se_node>& node_ptr) 
    {
        T_gaia * obj = nullptr;
        if (node_ptr) {
            auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
            auto it = s_gaia_cache.find(node_ptr->id);
            if (it != s_gaia_cache.end()){
                obj = dynamic_cast<T_gaia *>(it->second);
            }
            else {
                obj = new T_gaia();
                s_gaia_cache.insert(pair<gaia_id_t, GaiaBase *>(node_ptr->id, obj));
            }
            obj->set_fb(fb);
        }
        return obj;
    }
    void reset()
    {
        if (_copy){
            delete _copy;
        }
        if (_fbb) {
            delete _fbb;
        }
        _copy = nullptr;
        _fbb = nullptr;
    }

};

/*
struct IAirports_Readable
{
  virtual int64_t Gaia_id() = 0;
  virtual int32_t ap_id()  = 0;
  virtual const char *name() = 0;
  virtual const char *city() = 0;
  virtual const char *country() = 0;
  virtual const char *iata() = 0;
  virtual const char *icao() = 0;
  virtual double latitude() = 0;
  virtual double longitude() = 0;
  virtual int32_t altitude() = 0;
  virtual float timezone() = 0;
  virtual const char *dst() = 0;
  virtual const char *tztext() = 0;
  virtual const char *type() = 0;
  virtual const char *source() = 0;
};

struct IAirports_Writeable : public IAirports_Readable
{
  virtual void set_Gaia_id(int64_t id) = 0;
  virtual void set_ap_id(int32_t i) = 0;
  virtual void set_name(const char * s) = 0;
  virtual void set_city(const char * s) = 0;
  virtual void set_country(const char * s) = 0;
  virtual void set_iata(const char * s) = 0;
  virtual void set_icao(const char * s) = 0;
  virtual void set_latitude(double d) = 0;
  virtual void set_longitude(double d) = 0;
  virtual void set_altitude(int32_t i) = 0;
  virtual void set_timezone(float f) = 0;
  virtual void set_dst(const char * s) = 0;
  virtual void set_tztext(const char * s) = 0;
  virtual void set_type(const char * s) = 0;
  virtual void set_source(const char * s) = 0;
};

struct airports_wrap : public IAirports_Readable
{
    airports_wrap() : _fb(nullptr) {}
    void set_fb(const airports * fb) {_fb = fb;}
    const airports * get_fb() { return _fb; }
    int64_t Gaia_id() { return _fb->Gaia_id(); }
    int32_t ap_id()  { return _fb->ap_id(); }
    const char *name() { return _fb->name() ? _fb->name()->c_str() : nullptr; }
    const char *city() { return _fb->city() ? _fb->city()->c_str() : nullptr; }
    const char *country() { return _fb->country() ? _fb->country()->c_str() : nullptr; }  
    const char *iata() { return _fb->iata()  ? _fb->iata()->c_str() : nullptr; }
    const char *icao() { return _fb->icao() ? _fb->icao()->c_str() : nullptr; }    
    double latitude() { return _fb->latitude(); }
    double longitude() { return _fb->longitude(); }  
    int32_t altitude() { return _fb->altitude();}
    float timezone() { return _fb->timezone();};
    const char *dst() { return _fb->dst() ? _fb->dst()->c_str() : nullptr; }
    const char *tztext() { return _fb->tztext()  ? _fb->tztext()->c_str() : nullptr; }
    const char *type() { return _fb->type() ? _fb->type()->c_str() : nullptr; };
    const char *source() { return _fb->source() ? _fb->source()->c_str() : nullptr; }    
private:
    const airports * _fb;
};

struct airportsT_wrap : public IAirports_Writeable, public airportsT
{
    int64_t Gaia_id() { return airportsT::Gaia_id; }
    int32_t ap_id() { return airportsT::ap_id; }
    const char *name() { return airportsT::name.c_str(); }
    const char *city() { return airportsT::city.c_str(); }
    const char *country() { return airportsT::country.c_str(); }  
    const char *iata() { return airportsT::iata.c_str(); }
    const char *icao() { return airportsT::icao.c_str(); }    
    double latitude() { return airportsT::latitude; }
    double longitude() { return airportsT::longitude; }  
    int32_t altitude() { return airportsT::altitude; }
    float timezone() { return airportsT::timezone; };
    const char *dst() { return airportsT::dst.c_str(); }
    const char *tztext() { return airportsT::tztext.c_str(); }
    const char *type() { return airportsT::type.c_str(); };
    const char *source() { return airportsT::source.c_str(); }
    void set_Gaia_id(int64_t i){ airportsT::Gaia_id = i; }
    void set_ap_id(int32_t i){ airportsT::ap_id = i; }
    void set_name(const char * s) { airportsT::name = s; }
    void set_city(const char * s) { airportsT::city = s; }
    void set_country(const char * s) { airportsT::country = s; }  
    void set_iata(const char * s) { airportsT::iata = s; }
    void set_icao(const char * s) { airportsT::icao = s; }
    void set_latitude(double d){ airportsT::latitude = d; }
    void set_longitude(double d){ airportsT::longitude = d; }
    void set_altitude(int32_t i) { airportsT::altitude = i; }
    void set_timezone(float f){ airportsT::timezone = f; }
    void set_dst(const char * s) { airportsT::dst = s; }
    void set_tztext(const char * s) { airportsT::tztext = s; }
    void set_type(const char * s) { airportsT::type = s; }
    void set_source(const char * s) { airportsT::source = s; }     
};

struct Airport_rw : public GaiaRW<AirportDemo::kAirportsType, Airport_rw, airports_wrap, IAirports_Readable, airportsT_wrap, airports>
{
  int64_t Gaia_id() const { return _reader->Gaia_id(); }
  int32_t ap_id() const { return _reader->ap_id(); }
  const char *name() const { return _reader->name(); }
  const char *city() const { return _reader->city(); }
  const char *country() const { return _reader->country(); }  
  const char *iata() const { return _reader->iata(); }
  const char *icao() const { return _reader->icao(); }    
  double set_latitude() const { return _reader->latitude(); }
  double set_longitude() const { return _reader->longitude(); }  
  int32_t set_altitude() const { return _reader->altitude(); }
  float timezone() const { return _reader->timezone(); };
  const char *dst() const { return _reader->dst(); }
  const char *tztext() const{ return _reader->tztext(); }
  const char *type() const { return _reader->type(); };
  const char *source() const { return _reader->source(); }
  void set_Gaia_id(int64_t id){ writer()->set_Gaia_id(id); }
  void set_ap_id(int32_t i){ writer()->set_ap_id(i); }
  void set_name(const char * s) { writer()->set_name(s);}
  void set_city(const char * s) { writer()->set_city(s); }
  void set_country(const char * s) { writer()->set_country(s); }  
  void set_iata(const char * s) { writer()->set_iata(s); }
  void set_icao(const char * s) { writer()->set_icao(s); }
  void set_latitude(double d){ writer()->set_latitude(d); }
  void set_longitude(double d){ writer()->set_longitude(d); }
  void set_altitude(int32_t i){ writer()->set_altitude(i); }
  void set_timezone(float f){ writer()->set_timezone(f); }
  void set_dst(const char * s) { writer()->set_dst(s); }  
  void set_tztext(const char * s) { writer()->set_tztext(s); }
  void set_type(const char * s) { writer()->set_type(s); }
  void set_source(const char * s) { writer()->set_source(s); }    
};
*/

struct Airport_ZeroCopy_Mutable : public GaiaObj<AirportDemo::kAirportsType, Airport_ZeroCopy_Mutable, airports, airportsT>
{
    #define get(field) (_copy ? (_copy->field) : (_fb->field()))
    #define get_str(field) (_copy ? _copy->field.c_str() : _fb->field() ? _fb->field()->c_str() : nullptr)
    #define set(field, value) (copy_write()->field = value)

    int64_t Gaia_id() const { return get(Gaia_id); }
    int32_t ap_id() const { return get(ap_id); }
    const char * name() const { return get_str(name); }
    const char * city() const { return get_str(city); }
    const char * country() const { return get_str(country); }
    const char * iata() const { return get_str(iata); }
    const char * icao() const { return get_str(icao); }
    double latitude() const { return get(latitude); }
    double longitude() const { return get(longitude); }
    int32_t altitude() const { return get(altitude); }
    float timezone() const { return get(timezone); }
    const char * dst() const { return get_str(dst); }
    const char * tztext() const { return get_str(tztext); }
    const char * type() const { return get_str(type); }
    const char * source() const { return get_str(source); }
    void set_Gaia_id(uint32_t i) { set(Gaia_id, i); }
    void set_ap_id(uint32_t i) { set(ap_id, i); }
    void set_name(const char * s) { set(name, s); }
    void set_city(const char * s) { set(city, s); }
    void set_country(const char * s) { set(country, s); }
    void set_iata(const char * s) { set(iata, s); }
    void set_icao(const char * s) { set(icao, s); }
    void set_latitude(double d) { set(latitude, d); }
    void set_longitude(double d) { set(longitude, d); }
    void set_altitude(int32_t i) { set(altitude, i); }
}; //Airport zero copy mutable

class GaiaTx
{
public:
    GaiaTx(std::function<bool ()> fn) {
        gaia_se::begin_transaction();
        if (fn()) {
            gaia_se::commit_transaction();
        }
        else {
            gaia_se::rollback_transaction();
        }
    }
};


// **************************************************************************
// Access without any syntactic sugar.  This is zero-copy and immutable.
// Two areas to add syntax:  Flatbuffers and Gaia constructs
// **************************************************************************
uint32_t traverse_list_fb()
{
    uint32_t i = 0;

    GaiaTx([&]()
    {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);

        // dump the number of rows requested
        for (;node_ptr;node_ptr = node_ptr.find_next()) {
            auto row = flatbuffers::GetRoot<airports>(node_ptr->payload);
            int32_t ap_id = row->ap_id();
            ap_id++;
            if (row->iata()){
                auto len = strlen(row->iata()->c_str());
            }
            i++;
        }
        return true;
    });
    return i;
}

// **************************************************************************
// Access with gaia APIs that are external to the object
// **************************************************************************

uint32_t traverse_list_fb_api()
{
    uint32_t i = 0;
    GaiaTx([&]() {
        const airports * ap = gaia::GetFirst<airports, kAirportsType>();
        for(; ap; ap = gaia::GetNext(ap)) {
            int32_t ap_id = ap->ap_id();
            ap_id++;
            if (ap->iata()){
                auto len = strlen(ap->iata()->c_str());
            }
            i++;
        }
        return true;
    });
    return i;    
}

uint32_t update_list_fb()
{
    uint32_t i = 0;
    flatbuffers::FlatBufferBuilder b(128);
    GaiaTx([&]() 
    {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);

        // update the al_id of each row01
        for (;node_ptr;node_ptr = node_ptr.find_next()) {
            auto row = flatbuffers::GetRoot<airports>(node_ptr->payload);
            int32_t ap_id = row->ap_id();
            ap_id+= 100000;
            //auto node_ptr_u = gaia_se_node::open(row->Gaia_id());
            b.Finish(CreateairportsDirect(b, row->Gaia_id(), 0, 0, ap_id, 
                row->name() ? row->name()->c_str() : nullptr, 
                row->city() ? row->city()->c_str() : nullptr,
                row->country() ? row->country()->c_str() : nullptr,
                row->iata() ? row->iata()->c_str() : nullptr,
                row->icao() ? row->icao()->c_str() : nullptr,
                row->latitude(), row->longitude(), row->altitude(), row->timezone(),
                row->dst() ? row->dst()->c_str() : nullptr,
                row->tztext() ? row->tztext()->c_str() : nullptr,
                row->type() ? row->type()->c_str() : nullptr,
                row->source() ? row->source()->c_str() : nullptr));
            node_ptr.update_payload(b.GetSize(), b.GetBufferPointer());
            b.Clear();
            i++;
        }
        return true;
    });
    return i;
}

uint32_t update_list_fb_api()
{
    uint32_t i = 0;
    flatbuffers::FlatBufferBuilder b(128);
    GaiaTx([&]() 
    {
        const airports * ap = gaia::get_first_airport();
        for(; ap; ap = gaia::get_next_airport(ap)) {
            int32_t ap_id = ap->ap_id();
            airportsT * upd = gaia::get_updateable_airport(ap);
            upd->ap_id += 100000;
            gaia::update_airport(upd);
            i++;
        }
        return true;
    });
    return i;
}


// object access
uint32_t traverse_list_obj()
{
    uint32_t i = 0;
    airportsT obj;

    GaiaTx([&]()
    {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);

        // dump the number of rows requested
        for (;node_ptr;node_ptr = node_ptr.find_next()) {
            auto row = flatbuffers::GetRoot<airports>(node_ptr->payload);
            row->UnPackTo(&obj);
            int32_t ap_id = obj.ap_id;
            ap_id++;
            auto len = strlen(obj.iata.c_str());
            i++;
        }
        return true;
    });
    return i;
}

uint32_t update_list_obj()
{
    uint32_t i = 0;
    flatbuffers::FlatBufferBuilder b(128);
    GaiaTx([&]()
    {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(AirportDemo::kAirportsType);
        for (;node_ptr;node_ptr = node_ptr.find_next()) {
            auto row = flatbuffers::GetRoot<airports>(node_ptr->payload);
            airportsT * obj = row->UnPack();
            obj->ap_id += 10000;
            b.Finish(airports::Pack(b,obj));
            node_ptr.update_payload(b.GetSize(), b.GetBufferPointer());
            b.Clear();
            i++;
        }
        return true;
    });

    return i;
}

uint32_t traverse_list_gaia_mutable()
{
    uint32_t i = 0;
    Airport_Mutable obj;
    GaiaTx([&]() {
        Airport_Mutable * ap = Airport_Mutable::GetFirst(&obj);
        for(; ap; ap = ap->GetNext(&obj)) {
            int32_t ap_id = ap->ap_id;
            ap_id++;
            auto len = strlen(ap->iata.c_str());
            i++;
        }
        return true;
    });
    return i;
}

uint32_t update_list_gaia_mutable()
{
    uint32_t i = 0;
    Airport_Mutable obj;

    GaiaTx([&]() 
    {
        Airport_Mutable * ap = Airport_Mutable::GetFirst(&obj);
        for (; ap ; ap = ap->GetNext(&obj)) {
            ap->ap_id += 10000;
            ap->Update();
            i++;
        }
        return true;
    });
    return i;
}

uint32_t traverse_list_gaia_zerocopy_mutable()
{
    uint32_t i = 0;
    GaiaTx([&]() {
        Airport_ZeroCopy_Mutable * ap = Airport_ZeroCopy_Mutable::GetFirst();
        for(; ap; ap = ap->GetNext()) {
            int32_t ap_id = ap->ap_id();
            ap_id++;
            if (ap->iata()){
                auto len = strlen(ap->iata());
            }
            i++;
        }
        return true;
    });
    return i;
}

uint32_t traverse_list_gaia_zerocopy_mutable_reuse()
{
    uint32_t i = 0;
    Airport_ZeroCopy_Mutable obj;
    GaiaTx([&]() {
        Airport_ZeroCopy_Mutable * ap = Airport_ZeroCopy_Mutable::GetFirst(&obj);
        for(; ap; ap = ap->GetNext(&obj)) {
            int32_t ap_id = ap->ap_id();
            ap_id++;
            if (ap->iata()){
                auto len = strlen(ap->iata());
            }
            i++;
        }
        return true;
    });
    return i;
}

/*
uint32_t traverse_list_gaia_rw()
{
    uint32_t i = 0;
    GaiaTx([&]() {
        Airport_rw * ap = Airport_rw::GetFirst();
        for(; ap; ap = ap->GetNext()) {
            int32_t ap_id = ap->ap_id();
            ap_id++;
            if (ap->iata()){
                auto len = strlen(ap->iata());
            }
            i++;
        }
        return true;
    });
    return i;
}
*/

uint32_t update_list_gaia_zerocopy_mutable()
{
    uint32_t i = 0;

    GaiaTx([&]() 
    {
        Airport_ZeroCopy_Mutable * ap = Airport_ZeroCopy_Mutable::GetFirst();
        for (; ap ; ap = ap->GetNext()) {
            ap->set_ap_id(ap->ap_id() + 10000);
            ap->Update();
            i++;
        }
        return true;
    });
    return i;
}

/*
uint32_t update_list_gaia_rw()
{
    uint32_t i = 0;

    GaiaTx([&]() 
    {
        Airport_rw * ap = Airport_rw::GetFirst();
        for (; ap ; ap = ap->GetNext()) {
            ap->set_ap_id(ap->ap_id() + 10000);
            ap->Update();
            i++;
        }
        return true;
    });
    return i;
}
*/

uint32_t traverse_list_gaia_zerocopy()
{
    uint32_t i = 0;
    Airport_ZeroCopy obj;
    GaiaTx([&]() {
        Airport_ZeroCopy * ap = Airport_ZeroCopy::GetFirst(&obj);
        for(; ap; ap = ap->GetNext(&obj)) {
            int32_t ap_id = ap->ap_id();
            ap_id++;
            if (ap->iata()){
                auto len = strlen(ap->iata());
            }
            i++;
        }
        return true;
    });
    return i;    
}

uint32_t update_list_gaia_zerocopy()
{
    uint32_t i = 0;
    flatbuffers::FlatBufferBuilder b(128);
    Airport_ZeroCopy obj;

    GaiaTx([&]() 
    {
        Airport_ZeroCopy * ap = Airport_ZeroCopy::GetFirst(&obj);
        // update the al_id of each row01
        for (; ap; ap = ap->GetNext(&obj)) {
            int32_t ap_id = ap->ap_id();
            ap_id+= 100000;
            b.Finish(CreateairportsDirect(b, ap->Gaia_id(), 0, 0, ap_id, 
                ap->name(), 
                ap->city(),
                ap->country(),
                ap->iata(),
                ap->icao(),
                ap->latitude(),
                ap->longitude(),
                ap->altitude(),
                ap->timezone(),
                ap->dst(),
                ap->tztext(),
                ap->type(),
                ap->source()));
            
            auto node_ptr = gaia_se_node::open(ap->Gaia_id());
            node_ptr.update_payload(b.GetSize(), b.GetBufferPointer());
            b.Clear();
            i++;
        }
        return true;
    });
    return i;
}

int main (int argc, const char ** argv)
{
    (void)argc;
    (void)argv;
    gaia_mem_base::init(false);

    int64_t ns;
    int32_t rows;

    // *************************************************
    // read tests
    // *************************************************
    PerfTimer(ns, [&]() {
        rows = traverse_list_fb();
    });
    printf("fb:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_list_fb_api();
    });
    printf("fb api:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_list_obj();
    });
    printf("obj:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));
    
    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia_zerocopy();
    });
    printf("gaia zero copy:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia_mutable();
    });
    printf("gaia mutable:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia_zerocopy_mutable();
    });
    printf("gaia zero copy mutable:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia_zerocopy_mutable();
    });
    printf("gaia zero copy mutable after cache:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia_zerocopy_mutable_reuse();
    });
    printf("gaia zero copy mutable obj reuse:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    /*
    GaiaBase::s_gaia_cache.clear();
    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia_rw();
    });
    printf("gaia rw:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));
    PerfTimer(ns, [&]() {
        rows = traverse_list_gaia_rw();
    });
    printf("gaia rw after cache:  read %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));
    */

    // *************************************************
    // update tests
    // *************************************************
    PerfTimer(ns, [&]() {
        rows = update_list_fb();
    });
    printf("fb:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));
    PerfTimer(ns, [&]() {
        rows = update_list_fb_api();
    });
    printf("fb api:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = update_list_obj();
    });
    printf("obj:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));
    
    PerfTimer(ns, [&]() {
        rows = update_list_gaia_zerocopy();
    });
    printf("gaia zero copy:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    PerfTimer(ns, [&]() {
        rows = update_list_gaia_mutable();
    });
    printf("gaia mutable:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));

    GaiaBase::s_gaia_cache.clear();
    PerfTimer(ns, [&]() {
        rows = update_list_gaia_zerocopy_mutable();
    });
    printf("gaia zero copy mutable:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));    

    PerfTimer(ns, [&]() {
        rows = update_list_gaia_zerocopy_mutable();
    });
    printf("gaia zero copy mutable after cache:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));    

/*
    GaiaBase::s_gaia_cache.clear();
    PerfTimer(ns, [&]() {
        rows = update_list_gaia_rw();
    });
    printf("gaia rw:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));    

    PerfTimer(ns, [&]() {
        rows = update_list_gaia_rw();
    });
    printf("gaia fw after cache:  updated %u rows in %.2f ms\n", rows, PerfTimer::ns_ms(ns));    
*/
 
}
