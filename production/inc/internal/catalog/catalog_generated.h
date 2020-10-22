// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_CATALOG_GAIA_CATALOG_H_
#define FLATBUFFERS_GENERATED_CATALOG_GAIA_CATALOG_H_

#include "flatbuffers/flatbuffers.h"

namespace gaia {
namespace catalog {

struct gaia_rule;
struct gaia_ruleBuilder;
struct gaia_ruleT;

struct gaia_ruleset;
struct gaia_rulesetBuilder;
struct gaia_rulesetT;

struct gaia_field;
struct gaia_fieldBuilder;
struct gaia_fieldT;

struct gaia_table;
struct gaia_tableBuilder;
struct gaia_tableT;

struct gaia_database;
struct gaia_databaseBuilder;
struct gaia_databaseT;

struct gaia_ruleT : public flatbuffers::NativeTable {
  typedef gaia_rule TableType;
  gaia::direct_access::nullable_string_t name;
  gaia_ruleT() {
  }
};

struct gaia_rule FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef gaia_ruleT NativeTableType;
  typedef gaia_ruleBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           verifier.EndTable();
  }
  gaia_ruleT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(gaia_ruleT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<gaia_rule> Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_ruleT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct gaia_ruleBuilder {
  typedef gaia_rule Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(gaia_rule::VT_NAME, name);
  }
  explicit gaia_ruleBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  gaia_ruleBuilder &operator=(const gaia_ruleBuilder &);
  flatbuffers::Offset<gaia_rule> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<gaia_rule>(end);
    return o;
  }
};

inline flatbuffers::Offset<gaia_rule> Creategaia_rule(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0) {
  gaia_ruleBuilder builder_(_fbb);
  builder_.add_name(name);
  return builder_.Finish();
}

inline flatbuffers::Offset<gaia_rule> Creategaia_ruleDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return gaia::catalog::Creategaia_rule(
      _fbb,
      name__);
}

flatbuffers::Offset<gaia_rule> Creategaia_rule(flatbuffers::FlatBufferBuilder &_fbb, const gaia_ruleT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

struct gaia_rulesetT : public flatbuffers::NativeTable {
  typedef gaia_ruleset TableType;
  gaia::direct_access::nullable_string_t name;
  bool active_on_startup;
  gaia::direct_access::nullable_string_t table_ids;
  gaia::direct_access::nullable_string_t source_location;
  gaia::direct_access::nullable_string_t serial_stream;
  gaia_rulesetT()
      : active_on_startup(false) {
  }
};

struct gaia_ruleset FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef gaia_rulesetT NativeTableType;
  typedef gaia_rulesetBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_ACTIVE_ON_STARTUP = 6,
    VT_TABLE_IDS = 8,
    VT_SOURCE_LOCATION = 10,
    VT_SERIAL_STREAM = 12
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  bool active_on_startup() const {
    return GetField<uint8_t>(VT_ACTIVE_ON_STARTUP, 0) != 0;
  }
  const flatbuffers::String *table_ids() const {
    return GetPointer<const flatbuffers::String *>(VT_TABLE_IDS);
  }
  const flatbuffers::String *source_location() const {
    return GetPointer<const flatbuffers::String *>(VT_SOURCE_LOCATION);
  }
  const flatbuffers::String *serial_stream() const {
    return GetPointer<const flatbuffers::String *>(VT_SERIAL_STREAM);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<uint8_t>(verifier, VT_ACTIVE_ON_STARTUP) &&
           VerifyOffset(verifier, VT_TABLE_IDS) &&
           verifier.VerifyString(table_ids()) &&
           VerifyOffset(verifier, VT_SOURCE_LOCATION) &&
           verifier.VerifyString(source_location()) &&
           VerifyOffset(verifier, VT_SERIAL_STREAM) &&
           verifier.VerifyString(serial_stream()) &&
           verifier.EndTable();
  }
  gaia_rulesetT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(gaia_rulesetT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<gaia_ruleset> Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_rulesetT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct gaia_rulesetBuilder {
  typedef gaia_ruleset Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(gaia_ruleset::VT_NAME, name);
  }
  void add_active_on_startup(bool active_on_startup) {
    fbb_.AddElement<uint8_t>(gaia_ruleset::VT_ACTIVE_ON_STARTUP, static_cast<uint8_t>(active_on_startup), 0);
  }
  void add_table_ids(flatbuffers::Offset<flatbuffers::String> table_ids) {
    fbb_.AddOffset(gaia_ruleset::VT_TABLE_IDS, table_ids);
  }
  void add_source_location(flatbuffers::Offset<flatbuffers::String> source_location) {
    fbb_.AddOffset(gaia_ruleset::VT_SOURCE_LOCATION, source_location);
  }
  void add_serial_stream(flatbuffers::Offset<flatbuffers::String> serial_stream) {
    fbb_.AddOffset(gaia_ruleset::VT_SERIAL_STREAM, serial_stream);
  }
  explicit gaia_rulesetBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  gaia_rulesetBuilder &operator=(const gaia_rulesetBuilder &);
  flatbuffers::Offset<gaia_ruleset> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<gaia_ruleset>(end);
    return o;
  }
};

inline flatbuffers::Offset<gaia_ruleset> Creategaia_ruleset(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    bool active_on_startup = false,
    flatbuffers::Offset<flatbuffers::String> table_ids = 0,
    flatbuffers::Offset<flatbuffers::String> source_location = 0,
    flatbuffers::Offset<flatbuffers::String> serial_stream = 0) {
  gaia_rulesetBuilder builder_(_fbb);
  builder_.add_serial_stream(serial_stream);
  builder_.add_source_location(source_location);
  builder_.add_table_ids(table_ids);
  builder_.add_name(name);
  builder_.add_active_on_startup(active_on_startup);
  return builder_.Finish();
}

inline flatbuffers::Offset<gaia_ruleset> Creategaia_rulesetDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    bool active_on_startup = false,
    const char *table_ids = nullptr,
    const char *source_location = nullptr,
    const char *serial_stream = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto table_ids__ = table_ids ? _fbb.CreateString(table_ids) : 0;
  auto source_location__ = source_location ? _fbb.CreateString(source_location) : 0;
  auto serial_stream__ = serial_stream ? _fbb.CreateString(serial_stream) : 0;
  return gaia::catalog::Creategaia_ruleset(
      _fbb,
      name__,
      active_on_startup,
      table_ids__,
      source_location__,
      serial_stream__);
}

flatbuffers::Offset<gaia_ruleset> Creategaia_ruleset(flatbuffers::FlatBufferBuilder &_fbb, const gaia_rulesetT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

struct gaia_fieldT : public flatbuffers::NativeTable {
  typedef gaia_field TableType;
  gaia::direct_access::nullable_string_t name;
  uint8_t type;
  uint16_t repeated_count;
  uint16_t position;
  bool deprecated;
  bool active;
  gaia_fieldT()
      : type(0),
        repeated_count(0),
        position(0),
        deprecated(false),
        active(false) {
  }
};

struct gaia_field FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef gaia_fieldT NativeTableType;
  typedef gaia_fieldBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_TYPE = 6,
    VT_REPEATED_COUNT = 8,
    VT_POSITION = 10,
    VT_DEPRECATED = 12,
    VT_ACTIVE = 14
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  uint8_t type() const {
    return GetField<uint8_t>(VT_TYPE, 0);
  }
  uint16_t repeated_count() const {
    return GetField<uint16_t>(VT_REPEATED_COUNT, 0);
  }
  uint16_t position() const {
    return GetField<uint16_t>(VT_POSITION, 0);
  }
  bool deprecated() const {
    return GetField<uint8_t>(VT_DEPRECATED, 0) != 0;
  }
  bool active() const {
    return GetField<uint8_t>(VT_ACTIVE, 0) != 0;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<uint8_t>(verifier, VT_TYPE) &&
           VerifyField<uint16_t>(verifier, VT_REPEATED_COUNT) &&
           VerifyField<uint16_t>(verifier, VT_POSITION) &&
           VerifyField<uint8_t>(verifier, VT_DEPRECATED) &&
           VerifyField<uint8_t>(verifier, VT_ACTIVE) &&
           verifier.EndTable();
  }
  gaia_fieldT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(gaia_fieldT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<gaia_field> Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_fieldT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct gaia_fieldBuilder {
  typedef gaia_field Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(gaia_field::VT_NAME, name);
  }
  void add_type(uint8_t type) {
    fbb_.AddElement<uint8_t>(gaia_field::VT_TYPE, type, 0);
  }
  void add_repeated_count(uint16_t repeated_count) {
    fbb_.AddElement<uint16_t>(gaia_field::VT_REPEATED_COUNT, repeated_count, 0);
  }
  void add_position(uint16_t position) {
    fbb_.AddElement<uint16_t>(gaia_field::VT_POSITION, position, 0);
  }
  void add_deprecated(bool deprecated) {
    fbb_.AddElement<uint8_t>(gaia_field::VT_DEPRECATED, static_cast<uint8_t>(deprecated), 0);
  }
  void add_active(bool active) {
    fbb_.AddElement<uint8_t>(gaia_field::VT_ACTIVE, static_cast<uint8_t>(active), 0);
  }
  explicit gaia_fieldBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  gaia_fieldBuilder &operator=(const gaia_fieldBuilder &);
  flatbuffers::Offset<gaia_field> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<gaia_field>(end);
    return o;
  }
};

inline flatbuffers::Offset<gaia_field> Creategaia_field(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    uint8_t type = 0,
    uint16_t repeated_count = 0,
    uint16_t position = 0,
    bool deprecated = false,
    bool active = false) {
  gaia_fieldBuilder builder_(_fbb);
  builder_.add_name(name);
  builder_.add_position(position);
  builder_.add_repeated_count(repeated_count);
  builder_.add_active(active);
  builder_.add_deprecated(deprecated);
  builder_.add_type(type);
  return builder_.Finish();
}

inline flatbuffers::Offset<gaia_field> Creategaia_fieldDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    uint8_t type = 0,
    uint16_t repeated_count = 0,
    uint16_t position = 0,
    bool deprecated = false,
    bool active = false) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return gaia::catalog::Creategaia_field(
      _fbb,
      name__,
      type,
      repeated_count,
      position,
      deprecated,
      active);
}

flatbuffers::Offset<gaia_field> Creategaia_field(flatbuffers::FlatBufferBuilder &_fbb, const gaia_fieldT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

struct gaia_tableT : public flatbuffers::NativeTable {
  typedef gaia_table TableType;
  gaia::direct_access::nullable_string_t name;
  uint32_t type;
  bool is_system;
  gaia::direct_access::nullable_string_t binary_schema;
  gaia::direct_access::nullable_string_t serialization_template;
  gaia_tableT()
      : type(0),
        is_system(false) {
  }
};

struct gaia_table FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef gaia_tableT NativeTableType;
  typedef gaia_tableBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_TYPE = 6,
    VT_IS_SYSTEM = 8,
    VT_BINARY_SCHEMA = 10
    VT_SERIALIZATION_TEMPLATE = 12
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  uint32_t type() const {
    return GetField<uint32_t>(VT_TYPE, 0);
  }
  bool is_system() const {
    return GetField<uint8_t>(VT_IS_SYSTEM, 0) != 0;
  }
  const flatbuffers::String *binary_schema() const {
    return GetPointer<const flatbuffers::String *>(VT_BINARY_SCHEMA);
  }
  const flatbuffers::String *serialization_template() const {
    return GetPointer<const flatbuffers::String *>(VT_SERIALIZATION_TEMPLATE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<uint32_t>(verifier, VT_TYPE) &&
           VerifyField<uint8_t>(verifier, VT_IS_SYSTEM) &&
           VerifyOffset(verifier, VT_BINARY_SCHEMA) &&
           verifier.VerifyString(binary_schema()) &&
           VerifyOffset(verifier, VT_SERIALIZATION_TEMPLATE) &&
           verifier.VerifyString(serialization_template()) &&
           verifier.EndTable();
  }
  gaia_tableT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(gaia_tableT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<gaia_table> Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_tableT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct gaia_tableBuilder {
  typedef gaia_table Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(gaia_table::VT_NAME, name);
  }
  void add_type(uint32_t type) {
    fbb_.AddElement<uint32_t>(gaia_table::VT_TYPE, type, 0);
  }
  void add_is_system(bool is_system) {
    fbb_.AddElement<uint8_t>(gaia_table::VT_IS_SYSTEM, static_cast<uint8_t>(is_system), 0);
  }
  void add_binary_schema(flatbuffers::Offset<flatbuffers::String> binary_schema) {
    fbb_.AddOffset(gaia_table::VT_BINARY_SCHEMA, binary_schema);
  }
  void add_serialization_template(flatbuffers::Offset<flatbuffers::String> serialization_template) {
    fbb_.AddOffset(gaia_table::VT_SERIALIZATION_TEMPLATE, serialization_template);
  }
  explicit gaia_tableBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  gaia_tableBuilder &operator=(const gaia_tableBuilder &);
  flatbuffers::Offset<gaia_table> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<gaia_table>(end);
    return o;
  }
};

inline flatbuffers::Offset<gaia_table> Creategaia_table(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
<<<<<<< HEAD
    uint32_t type = 0,
    bool is_system = false,
=======
    bool is_log = false,
>>>>>>> 430e678152762909f9d0aacd111b87261dd5441d
    flatbuffers::Offset<flatbuffers::String> binary_schema = 0,
    flatbuffers::Offset<flatbuffers::String> serialization_template = 0) {
  gaia_tableBuilder builder_(_fbb);
  builder_.add_serialization_template(serialization_template);
  builder_.add_binary_schema(binary_schema);
  builder_.add_type(type);
  builder_.add_name(name);
  builder_.add_is_system(is_system);
  return builder_.Finish();
}

inline flatbuffers::Offset<gaia_table> Creategaia_tableDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    uint32_t type = 0,
    bool is_system = false,
    const char *binary_schema = nullptr,
    const char *serialization_template = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto binary_schema__ = binary_schema ? _fbb.CreateString(binary_schema) : 0;
  auto serialization_template__ = serialization_template ? _fbb.CreateString(serialization_template) : 0;
  return gaia::catalog::Creategaia_table(
      _fbb,
      name__,
      type,
      is_system,
      binary_schema__,
      serialization_template__);
}

flatbuffers::Offset<gaia_table> Creategaia_table(flatbuffers::FlatBufferBuilder &_fbb, const gaia_tableT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

struct gaia_databaseT : public flatbuffers::NativeTable {
  typedef gaia_database TableType;
  gaia::direct_access::nullable_string_t name;
  gaia_databaseT() {
  }
};

struct gaia_database FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef gaia_databaseT NativeTableType;
  typedef gaia_databaseBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           verifier.EndTable();
  }
  gaia_databaseT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(gaia_databaseT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<gaia_database> Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_databaseT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct gaia_databaseBuilder {
  typedef gaia_database Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(gaia_database::VT_NAME, name);
  }
  explicit gaia_databaseBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  gaia_databaseBuilder &operator=(const gaia_databaseBuilder &);
  flatbuffers::Offset<gaia_database> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<gaia_database>(end);
    return o;
  }
};

inline flatbuffers::Offset<gaia_database> Creategaia_database(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0) {
  gaia_databaseBuilder builder_(_fbb);
  builder_.add_name(name);
  return builder_.Finish();
}

inline flatbuffers::Offset<gaia_database> Creategaia_databaseDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return gaia::catalog::Creategaia_database(
      _fbb,
      name__);
}

flatbuffers::Offset<gaia_database> Creategaia_database(flatbuffers::FlatBufferBuilder &_fbb, const gaia_databaseT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

inline gaia_ruleT *gaia_rule::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  std::unique_ptr<gaia::catalog::gaia_ruleT> _o = std::unique_ptr<gaia::catalog::gaia_ruleT>(new gaia_ruleT());
  UnPackTo(_o.get(), _resolver);
  return _o.release();
}

inline void gaia_rule::UnPackTo(gaia_ruleT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = name(); if (_e) _o->name = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
}

inline flatbuffers::Offset<gaia_rule> gaia_rule::Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_ruleT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return Creategaia_rule(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<gaia_rule> Creategaia_rule(flatbuffers::FlatBufferBuilder &_fbb, const gaia_ruleT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const gaia_ruleT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _name = _o->name.empty() ? 0 : _fbb.CreateString(_o->name);
  return gaia::catalog::Creategaia_rule(
      _fbb,
      _name);
}

inline gaia_rulesetT *gaia_ruleset::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  std::unique_ptr<gaia::catalog::gaia_rulesetT> _o = std::unique_ptr<gaia::catalog::gaia_rulesetT>(new gaia_rulesetT());
  UnPackTo(_o.get(), _resolver);
  return _o.release();
}

inline void gaia_ruleset::UnPackTo(gaia_rulesetT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = name(); if (_e) _o->name = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
  { auto _e = active_on_startup(); _o->active_on_startup = _e; }
  { auto _e = table_ids(); if (_e) _o->table_ids = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
  { auto _e = source_location(); if (_e) _o->source_location = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
  { auto _e = serial_stream(); if (_e) _o->serial_stream = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
}

inline flatbuffers::Offset<gaia_ruleset> gaia_ruleset::Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_rulesetT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return Creategaia_ruleset(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<gaia_ruleset> Creategaia_ruleset(flatbuffers::FlatBufferBuilder &_fbb, const gaia_rulesetT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const gaia_rulesetT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _name = _o->name.empty() ? 0 : _fbb.CreateString(_o->name);
  auto _active_on_startup = _o->active_on_startup;
  auto _table_ids = _o->table_ids.empty() ? 0 : _fbb.CreateString(_o->table_ids);
  auto _source_location = _o->source_location.empty() ? 0 : _fbb.CreateString(_o->source_location);
  auto _serial_stream = _o->serial_stream.empty() ? 0 : _fbb.CreateString(_o->serial_stream);
  return gaia::catalog::Creategaia_ruleset(
      _fbb,
      _name,
      _active_on_startup,
      _table_ids,
      _source_location,
      _serial_stream);
}

inline gaia_fieldT *gaia_field::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  std::unique_ptr<gaia::catalog::gaia_fieldT> _o = std::unique_ptr<gaia::catalog::gaia_fieldT>(new gaia_fieldT());
  UnPackTo(_o.get(), _resolver);
  return _o.release();
}

inline void gaia_field::UnPackTo(gaia_fieldT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = name(); if (_e) _o->name = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
  { auto _e = type(); _o->type = _e; }
  { auto _e = repeated_count(); _o->repeated_count = _e; }
  { auto _e = position(); _o->position = _e; }
  { auto _e = deprecated(); _o->deprecated = _e; }
  { auto _e = active(); _o->active = _e; }
}

inline flatbuffers::Offset<gaia_field> gaia_field::Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_fieldT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return Creategaia_field(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<gaia_field> Creategaia_field(flatbuffers::FlatBufferBuilder &_fbb, const gaia_fieldT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const gaia_fieldT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _name = _o->name.empty() ? 0 : _fbb.CreateString(_o->name);
  auto _type = _o->type;
  auto _repeated_count = _o->repeated_count;
  auto _position = _o->position;
  auto _deprecated = _o->deprecated;
  auto _active = _o->active;
  return gaia::catalog::Creategaia_field(
      _fbb,
      _name,
      _type,
      _repeated_count,
      _position,
      _deprecated,
      _active);
}

inline gaia_tableT *gaia_table::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  std::unique_ptr<gaia::catalog::gaia_tableT> _o = std::unique_ptr<gaia::catalog::gaia_tableT>(new gaia_tableT());
  UnPackTo(_o.get(), _resolver);
  return _o.release();
}

inline void gaia_table::UnPackTo(gaia_tableT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = name(); if (_e) _o->name = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
  { auto _e = type(); _o->type = _e; }
  { auto _e = is_system(); _o->is_system = _e; }
  { auto _e = binary_schema(); if (_e) _o->binary_schema = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
  { auto _e = serialization_template(); if (_e) _o->serialization_template = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
}

inline flatbuffers::Offset<gaia_table> gaia_table::Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_tableT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return Creategaia_table(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<gaia_table> Creategaia_table(flatbuffers::FlatBufferBuilder &_fbb, const gaia_tableT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const gaia_tableT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _name = _o->name.empty() ? 0 : _fbb.CreateString(_o->name);
  auto _type = _o->type;
  auto _is_system = _o->is_system;
  auto _binary_schema = _o->binary_schema.empty() ? 0 : _fbb.CreateString(_o->binary_schema);
  auto _serialization_template = _o->serialization_template.empty() ? 0 : _fbb.CreateString(_o->serialization_template);
  return gaia::catalog::Creategaia_table(
      _fbb,
      _name,
      _type,
      _is_system,
      _binary_schema,
      _serialization_template);
}

inline gaia_databaseT *gaia_database::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  std::unique_ptr<gaia::catalog::gaia_databaseT> _o = std::unique_ptr<gaia::catalog::gaia_databaseT>(new gaia_databaseT());
  UnPackTo(_o.get(), _resolver);
  return _o.release();
}

inline void gaia_database::UnPackTo(gaia_databaseT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = name(); if (_e) _o->name = gaia::direct_access::nullable_string_t(_e->c_str(), _e->size()); }
}

inline flatbuffers::Offset<gaia_database> gaia_database::Pack(flatbuffers::FlatBufferBuilder &_fbb, const gaia_databaseT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return Creategaia_database(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<gaia_database> Creategaia_database(flatbuffers::FlatBufferBuilder &_fbb, const gaia_databaseT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const gaia_databaseT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _name = _o->name.empty() ? 0 : _fbb.CreateString(_o->name);
  return gaia::catalog::Creategaia_database(
      _fbb,
      _name);
}

}  // namespace catalog
}  // namespace gaia

#endif  // FLATBUFFERS_GENERATED_CATALOG_GAIA_CATALOG_H_
