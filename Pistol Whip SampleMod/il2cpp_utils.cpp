#include "il2cpp_utils.hpp"
#include <sstream>
#include "logger.h"

// Many implementaions from https://github.com/sc2ad/pistol-whip-hook/blob/fd7edc3d1d39d231e43c1430dbf4336045a056cc/shared/utils/il2cpp-utils.cpp
namespace il2cpp_utils {
	using namespace std;
	const MethodInfo* GetMethod(Il2CppClass* klass, std::string_view methodName, int argsCount) {
		if (!klass) return nullptr;
		auto methodInfo = il2cpp_functions::class_get_method_from_name(klass, methodName.data(), argsCount);
		if (!methodInfo) {
			LOG("ERROR: could not find method %s with %i parameters in class %s (namespace '%s')!", methodName.data(),
				argsCount, il2cpp_functions::class_get_name(klass), il2cpp_functions::class_get_namespace(klass));
			return nullptr;
		}
		return methodInfo;
	}

	const MethodInfo* GetMethod(std::string_view nameSpace, std::string_view className, std::string_view methodName, int argsCount) {
		return GetMethod(GetClassFromName(nameSpace.data(), className.data()), methodName, argsCount);
	}

	// Returns a legible string from an Il2CppException*
	std::string ExceptionToString(Il2CppException* exp) {
		char msg[EXCEPTION_MESSAGE_SIZE];
		il2cpp_functions::format_exception(exp, msg, EXCEPTION_MESSAGE_SIZE);
		// auto exception_message = csstrtostr(exp->message);
		// return to_utf8(exception_message);
		return msg;
	}

	FieldInfo* FindField(Il2CppClass* klass, std::string_view fieldName) {
		if (!klass) return nullptr;
		auto field = il2cpp_functions::class_get_field_from_name(klass, fieldName.data());
		if (!field) {
			LOG("could not find field %s in class %s (namespace '%s')!", fieldName.data(),
				il2cpp_functions::class_get_name(klass), il2cpp_functions::class_get_namespace(klass));
		}
		return field;
	}

	bool SetFieldValue(Il2CppObject* instance, FieldInfo* field, void* value) {
		if (!field) {
			LOG("il2cpp_utils: SetFieldValue: Null field parameter!");
			return false;
		}
		if (instance) {
			il2cpp_functions::field_set_value(instance, field, value);
		}
		else { // Fallback to perform a static field set
			il2cpp_functions::field_static_set_value(field, value);
		}
		return true;
	}


	bool SetFieldValue(Il2CppClass* klass, std::string_view fieldName, void* value) {
		if (!klass) {
			LOG("il2cpp_utils: SetFieldValue: Null klass parameter!");
			return false;
		}
		auto field = FindField(klass, fieldName);
		if (!field) return false;
		return SetFieldValue(nullptr, field, value);
	}

	bool SetFieldValue(Il2CppObject* instance, std::string_view fieldName, void* value) {
		if (!instance) {
			LOG("il2cpp_utils: SetFieldValue: Null instance parameter!");
			return false;
		}
		auto klass = il2cpp_functions::object_get_class(instance);
		if (!klass) {
			LOG("il2cpp_utils: SetFieldValue: Could not find object class!");
			return false;
		}
		auto field = FindField(klass, fieldName);
		if (!field) return false;
		return SetFieldValue(instance, field, value);
	}

	Il2CppClass* GetClassFromName(const char* name_space, const char* type_name) {

		auto dom = il2cpp_functions::domain_get();
		if (!dom) {
			LOG("ERROR: GetClassFromName: Could not get domain!");
			return nullptr;
		}
		size_t assemb_count;
		const Il2CppAssembly** allAssemb = il2cpp_functions::domain_get_assemblies(dom, &assemb_count);

		for (int i = 0; i < assemb_count; i++) {
			auto assemb = allAssemb[i];
			auto img = il2cpp_functions::assembly_get_image(assemb);
			if (!img) {
				LOG("ERROR: Assembly with name: %s has a null image!", assemb->aname.name);
				continue;
			}
			auto klass = il2cpp_functions::class_from_name(img, name_space, type_name);
			if (klass) {
				return klass;
			}
		}
		LOG("ERROR: il2cpp_utils: GetClassFromName: Could not find class with namepace: %s and name: %s", name_space, type_name);
		return nullptr;
	}

	
}

