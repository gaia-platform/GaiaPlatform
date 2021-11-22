/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/////////////////////////////////////////////
// Modifications Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
// independent from idl_parser, since this code is not needed for most clients
#include <unordered_set>

#include "flatbuffers/code_generators.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/flatc.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace flatbuffers 
{
    // Pedantic warning free version of toupper().
    inline char ToUpper(char c) 
    {
        return static_cast<char>(::toupper(static_cast<unsigned char>(c)));
    }

    static std::string GeneratedFileName(const std::string &path,
        const std::string &file_name) 
    {
        return path + file_name + "_gaia_generated.h";
    }

    static std::string GeneratedCPPFileName(const std::string &file_name) 
    {
        return file_name + "_generated.h";
    }

    static std::string GenIncludeGuard(const std::string &file_name,
        const Namespace &name_space, const std::string &postfix= "")  
    {
        // Generate include guard.
        std::string guard = file_name;
        // Remove any non-alpha-numeric characters that may appear in a filename.
        struct IsAlnum 
        {
            bool operator()(char c) const 
            { 
                return !is_alnum(c); 
            }
        };
        guard.erase(std::remove_if(guard.begin(), guard.end(), IsAlnum()),
            guard.end());

        guard = "FLATBUFFERS_GENERATED_GAIA_" + guard;
        guard += "_";
        
        // For further uniqueness, also add the namespace.
        for (auto it = name_space.components.begin();
            it != name_space.components.end(); ++it) 
        {
            guard += *it + "_";
        }

        // Anything extra to add to the guard?
        if (!postfix.empty()) 
        {
            guard += postfix + "_";
        }
        guard += "H_";
        std::transform(guard.begin(), guard.end(), guard.begin(), ToUpper);
        return guard;
    }

    static std::string CapitalizeString(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(),++result.begin(),result.begin(),ToUpper);
        return result;
    }

    namespace gaiacpp 
    {

        class GaiaCppGenerator : public BaseGenerator 
        {
        public:
            GaiaCppGenerator(const Parser &parser, const std::string &path,
               const std::string &file_name, IDLOptions options)
                : BaseGenerator(parser, path, file_name, "", "::", "h"),
                cur_name_space_(nullptr), opts_(options) 
            {
                static const char *const keywords[] = 
                {
                    "alignas",
                    "alignof",
                    "and",
                    "and_eq",
                    "asm",
                    "atomic_cancel",
                    "atomic_commit",
                    "atomic_noexcept",
                    "auto",
                    "bitand",
                    "bitor",
                    "bool",
                    "break",
                    "case",
                    "catch",
                    "char",
                    "char16_t",
                    "char32_t",
                    "class",
                    "compl",
                    "concept",
                    "const",
                    "constexpr",
                    "const_cast",
                    "continue",
                    "co_await",
                    "co_return",
                    "co_yield",
                    "decltype",
                    "default",
                    "delete",
                    "do",
                    "double",
                    "dynamic_cast",
                    "else",
                    "enum",
                    "explicit",
                    "export",
                    "extern",
                    "false",
                    "float",
                    "for",
                    "friend",
                    "goto",
                    "if",
                    "import",
                    "inline",
                    "int",
                    "long",
                    "module",
                    "mutable",
                    "namespace",
                    "new",
                    "noexcept",
                    "not",
                    "not_eq",
                    "nullptr",
                    "operator",
                    "or",
                    "or_eq",
                    "private",
                    "protected",
                    "public",
                    "register",
                    "reinterpret_cast",
                    "requires",
                    "return",
                    "short",
                    "signed",
                    "sizeof",
                    "static",
                    "static_assert",
                    "static_cast",
                    "struct",
                    "switch",
                    "synchronized",
                    "template",
                    "this",
                    "thread_local",
                    "throw",
                    "true",
                    "try",
                    "typedef",
                    "typeid",
                    "typename",
                    "union",
                    "unsigned",
                    "using",
                    "virtual",
                    "void",
                    "volatile",
                    "wchar_t",
                    "while",
                    "xor",
                    "xor_eq",
                    nullptr,
                };
                for (auto keyword = keywords; *keyword; keyword++) 
                {
                    keywords_.insert(*keyword);
                }
            }
  
            const std::string EscapeKeyword(const std::string &name) const 
            {
                return keywords_.find(name) == keywords_.end() ? name : name + "_";
            }

            const std::string Name(const Definition &def) const 
            {
                return EscapeKeyword(def.name);
            }

            const std::string Name(const EnumVal &ev) const 
            { 
                return EscapeKeyword(ev.name); 
            }

            std::string GenFieldOffsetName(const FieldDef &field) 
            {
                std::string uname = Name(field);
                std::transform(uname.begin(), uname.end(), uname.begin(), ToUpper);
                return "VT_" + uname;
            }


            // Iterate through all definitions we haven't generate code for (enums,
            // structs, and tables) and output them to a single file.
            bool generate()
            {
                code_.Clear();
                code_ += "// " + std::string(FlatBuffersGeneratedWarning()) + "\n\n";

                const auto include_guard = GenIncludeGuard(file_name_, 
                    *parser_.current_namespace_);
                code_ += "#ifndef " + include_guard;
                code_ += "#define " + include_guard;
                code_ += "";

                code_ += "#include \"edc_object.hpp\"";
                code_ += "#include \"" + GeneratedCPPFileName(file_name_) + "\"";
                
                code_ += "";
                FLATBUFFERS_ASSERT(!cur_name_space_);

                code_ += "using namespace std;";
                code_ += "using namespace gaia::common;";
                code_ += "using namespace gaia::direct_access;";
                code_ += "";


                if (cur_name_space_) 
                {
                    SetNameSpace(nullptr);
                }

                uint64_t currentObjectTypeValue = opts_.gaia_type_initial_value;

                // Generate class.
                for (auto it = parser_.structs_.vec.cbegin();
                    it != parser_.structs_.vec.cend(); ++it) 
                {
                    const auto &struct_def = **it;
                    if (!struct_def.fixed && !struct_def.generated) 
                    {
                        SetNameSpace(struct_def.defined_namespace);
                        GenClass(struct_def, currentObjectTypeValue);
                        code_ += "";
                        currentObjectTypeValue++;
                    }
                }

                if (cur_name_space_) 
                {
                    SetNameSpace(nullptr);
                }
    
                // Close the include guard.
                code_ += "#endif  // " + include_guard;

                const auto file_path = flatbuffers::GeneratedFileName(path_, file_name_);
                const auto final_code = code_.ToString();

                // Save the file.
                return SaveFile(file_path.c_str(), final_code, false);
            }

        private:
            CodeWriter code_;
            std::unordered_set<std::string> keywords_;

            // This tracks the current namespace so we can insert namespace declarations.
            const Namespace *cur_name_space_;

            const IDLOptions opts_;

            const std::string CurrentNamespaceString() const
            {
                const Namespace *ns = CurrentNameSpace();
                std::string result = "";
                if (ns == nullptr)
                {
                    return result;
                }
    
                // Open namespace parts to reach the ns namespace               
                for (unsigned long j = 0; j != ns->components.size(); ++j) 
                {
                    if (!result.empty())
                    {
                        result += "::";
                    }
                    result += ns->components[j];
                }

                return result;
            }

            const Namespace *CurrentNameSpace() const 
            { 
                return cur_name_space_; 
            }

            void GenComment(const std::vector<std::string> &dc, 
                    const char *prefix = "") 
            {
                std::string text;
                ::flatbuffers::GenComment(dc, &text, nullptr, prefix);
                code_ += text + "\\";
            }

            // Return a C++ type from the table in idl.h
            const std::string GenTypeBasic(const Type &type, bool user_facing_type) const 
            {
                // clang-format off
                static const char *const ctypename[] = 
                {
                #define FLATBUFFERS_TD(ENUM, IDLTYPE, CTYPE, ...) \
                    #CTYPE,
                    FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
                #undef FLATBUFFERS_TD
                };
                // clang-format on
                if (user_facing_type) 
                {
                    if (type.enum_def) 
                    {
                        return WrapInNameSpace(*type.enum_def);
                    }

                    if (type.base_type == BASE_TYPE_BOOL) 
                    {
                        return "bool";
                    }
                }
                return ctypename[type.base_type];
            }

            static std::string NativeName(const std::string &name, const StructDef *sd,
                const IDLOptions &opts) 
            {
                return sd && !sd->fixed ? opts.object_prefix + name + opts.object_suffix
                    : name;
            }

            const std::string &PtrType(const FieldDef *field) const 
            {
                auto attr = field ? field->attributes.Lookup("cpp_ptr_type") : nullptr;
                return attr ? attr->constant : opts_.cpp_object_api_pointer_type;
            }

            const std::string NativeString() const 
            {
                return "const char *";
            }

            std::string GenTypeNativePtr(const std::string &type, const FieldDef *field,
                bool is_constructor) const 
            {
                auto &ptr_type = PtrType(field);
                if (ptr_type != "naked") 
                {
                    return (ptr_type != "default_ptr_type"
                        ? ptr_type
                        : opts_.cpp_object_api_pointer_type) +
                            "<" + type + ">";
                }
                else
                { 
                    if (is_constructor) 
                    {
                        return "";
                    } 
                    else 
                    {
                        return type + " *";
                    }
                }
            }

            const std::string GenTypeNative(const Type &type, bool invector,
                const FieldDef &field) const 
            {
                switch (type.base_type) 
                {
                    case BASE_TYPE_STRING: 
                    {
                        return NativeString();
                    }
                    case BASE_TYPE_VECTOR: 
                    {
                        const auto type_name = GenTypeNative(type.VectorType(), true, field);
                        if (type.struct_def &&
                            type.struct_def->attributes.Lookup("native_custom_alloc")) 
                        {
                            auto native_custom_alloc =
                                type.struct_def->attributes.Lookup("native_custom_alloc");
                            return "std::vector<" + type_name + "," +
                                native_custom_alloc->constant + "<" + type_name + ">>";
                        } 
                        else
                        {
                            return "std::vector<" + type_name + ">";
                        }
                    }
                    case BASE_TYPE_STRUCT: 
                    {
                        auto type_name = WrapInNameSpace(*type.struct_def);
                        if (IsStruct(type)) 
                        {
                            auto native_type = type.struct_def->attributes.Lookup("native_type");
                            if (native_type) 
                            { 
                                type_name = native_type->constant; 
                            }

                            if (invector || field.native_inline) 
                            {
                                return type_name;
                            }
                            else 
                            {
                                return GenTypeNativePtr(type_name, &field, false);
                            }
                        } 
                        else
                        {
                            return GenTypeNativePtr(NativeName(type_name, type.struct_def, opts_),
                                &field, false);
                        }
                    }
                    case BASE_TYPE_UNION: 
                    {
                        auto type_name = WrapInNameSpace(*type.enum_def);
                        return type_name + "Union";
                    }
                    default: 
                    {
                        return GenTypeBasic(type, true);
                    }
                }
            }
   
            // Generate a Zero Copy class with Setters
            void GenClass(const StructDef &struct_def, const uint64_t currentObjectTypeValue) 
            {                
                // Generate an accessor struct, with methods of the form:
                // type name() const { return GetField<type>(offset, defaultval); }
                GenComment(struct_def.doc_comment);

                code_.SetValue("STRUCT_NAME", Name(struct_def));
                code_.SetValue("CLASS_NAME", CapitalizeString(Name(struct_def)));

                code_ += "struct {{CLASS_NAME}} : public edc_object_t<" + NumToString(currentObjectTypeValue) + 
                    ",{{CLASS_NAME}},{{STRUCT_NAME}},{{STRUCT_NAME}}T, 0>{";
    
                std::string params = "";
                std::string param_Values = "";

                bool has_string_or_vector_fields = false;
           
                // Generate the accessors.
                for (auto it = struct_def.fields.vec.begin();
                    it != struct_def.fields.vec.end(); ++it) 
                {
                    const auto &field = **it;
                    if (field.deprecated) 
                    {
                        // Deprecated fields won't be accessible.
                        continue;
                    }

                    // Track whether any field has a string or vector type
                    // to generate the correct Create{{STRUCT_NAME}} call below.
                    if (field.value.type.base_type == BASE_TYPE_STRING 
                        || field.value.type.base_type == BASE_TYPE_VECTOR)
                    {
                        has_string_or_vector_fields = true;
                    }

                    if (!params.empty())
                    {
                        params += ",";
                    }

                    if (!param_Values.empty())
                    {
                        param_Values += ",";
                    }

                    params += GenTypeNative(field.value.type,false, field) + " " +  Name(field) + "_val";
                    param_Values  += Name(field) + "_val";

                    code_.SetValue("FIELD_NAME", Name(field));
                    code_.SetValue("FIELD_TYPE", GenTypeNative(field.value.type,false, field));
                    code_.SetValue("OFFSET", GenFieldOffsetName(field));
     
                    if (field.value.type.base_type == BASE_TYPE_STRING)
                    {
                        code_ += 
                            "{{FIELD_TYPE}} {{FIELD_NAME}} () const { return GET_STR({{FIELD_NAME}});}";
                    }
                    else
                    {
                        code_ += 
                            "{{FIELD_TYPE}} {{FIELD_NAME}} () const { return GET({{FIELD_NAME}});}";
                    }
                }

                code_ += 
                    "using edc_object_t::insert_row;";

                // If the flatbuffer has a string or vector column then 
                // generate a call to Create{{STRUCT_NAME}}Direct.  Otherwise 
                // just generate Create{{STRUCT_NAME}}.
                code_.SetValue("CREATE_SUFFIX", 
                    has_string_or_vector_fields ? "Direct" : "");
                code_ += "static gaia_id_t insert_row (" + params + "){\n"
                    "flatbuffers::FlatBufferBuilder b(128);\n"
                    "b.Finish(Create{{STRUCT_NAME}}{{CREATE_SUFFIX}}(b, " + param_Values + "));\n"
                    "return edc_object_t::insert_row(b);\n"
                    "}";

                code_ += "private:";
                code_ += "friend struct edc_object_t<" + NumToString(currentObjectTypeValue) +  
                    ",{{CLASS_NAME}},{{STRUCT_NAME}},{{STRUCT_NAME}}T, 0>;";
                code_ += "{{CLASS_NAME}}(gaia_id_t id) : edc_object_t(id, \"{{CLASS_NAME}}\") {}";
                
                code_ += "};";

                // Generate pointer to writer
                code_ += "typedef edc_writer_t<" + NumToString(currentObjectTypeValue) +
                    ",{{CLASS_NAME}},{{STRUCT_NAME}},{{STRUCT_NAME}}T, 0> {{CLASS_NAME}}_writer;";
            }

            // Set up the correct namespace. Only open a namespace if the existing one is
            // different (closing/opening only what is necessary).
            //
            // The file must start and end with an empty (or null) namespace so that
            // namespaces are properly opened and closed.
            void SetNameSpace(const Namespace *ns) 
            {
                if (cur_name_space_ == ns) 
                { 
                    return; 
                }

                // Compute the size of the longest common namespace prefix.
                // If cur_name_space is A::B::C::D and ns is A::B::E::F::G,
                // the common prefix is A::B:: and we have old_size = 4, new_size = 5
                // and common_prefix_size = 2
                size_t old_size = cur_name_space_ ? cur_name_space_->components.size() : 0;
                size_t new_size = ns ? ns->components.size() : 0;

                size_t common_prefix_size = 0;
                while (common_prefix_size < old_size && common_prefix_size < new_size &&
                    ns->components[common_prefix_size] ==
                    cur_name_space_->components[common_prefix_size]) 
                {
                    common_prefix_size++;
                }

                // Close cur_name_space in reverse order to reach the common prefix.
                // In the previous example, D then C are closed.
                for (size_t j = old_size; j > common_prefix_size; --j) 
                {
                    code_ += "}  // namespace " + cur_name_space_->components[j - 1];
                }

                if (old_size != common_prefix_size) 
                { 
                    code_ += ""; 
                }

                // open namespace parts to reach the ns namespace
                // in the previous example, E, then F, then G are opened
                for (auto j = common_prefix_size; j != new_size; ++j) 
                {
                    code_ += "namespace " + ns->components[j] + " {";
                }
                if (new_size != common_prefix_size) 
                { 
                    code_ += ""; 
                }

                cur_name_space_ = ns;
            }
        };

    }  // namespace gaiacpp

    bool GenerateGaiaCPP(const Parser &parser, const std::string &path,
        const std::string &file_name) 
    {  
        gaiacpp::GaiaCppGenerator generator(parser, path, file_name, parser.opts);
        return generator.generate();
    }

    std::string GaiaCPPMakeRule(const Parser &parser, const std::string &path,
        const std::string &file_name) 
    {
        const auto filebase =
            flatbuffers::StripPath(flatbuffers::StripExtension(file_name));
        const auto included_files = parser.GetIncludedFilesRecursive(file_name);
        std::string make_rule = GeneratedFileName(path, filebase) + ": ";
        for (auto it = included_files.begin(); it != included_files.end(); ++it) 
        {
            make_rule += " " + *it;
        }
        return make_rule;
    }

}  // namespace flatbuffers
