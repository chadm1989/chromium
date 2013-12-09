{% extends 'interface_base.cpp' %}


{##############################################################################}
{% macro attribute_configuration(attribute) %}
{% set getter_callback =
       '%sV8Internal::%sAttributeGetterCallback' %
            (interface_name, attribute.name)
       if not attribute.constructor_type else
       '{0}V8Internal::{0}ConstructorGetter'.format(interface_name) %}
{% set getter_callback_for_main_world =
       '%sV8Internal::%sAttributeGetterCallbackForMainWorld' %
            (interface_name, attribute.name)
       if attribute.is_per_world_bindings else '0' %}
{% set setter_callback = attribute.setter_callback %}
{% set setter_callback_for_main_world =
       '%sV8Internal::%sAttributeSetterCallbackForMainWorld' %
           (interface_name, attribute.name)
       if attribute.is_per_world_bindings and not attribute.is_read_only else '0' %}
{% set wrapper_type_info =
       'const_cast<WrapperTypeInfo*>(&V8%s::wrapperTypeInfo)' %
            attribute.constructor_type
        if attribute.constructor_type else '0' %}
{% set access_control = 'static_cast<v8::AccessControl>(%s)' %
                        ' | '.join(attribute.access_control_list) %}
{% set property_attribute = 'static_cast<v8::PropertyAttribute>(%s)' %
                            ' | '.join(attribute.property_attributes) %}
{% set on_prototype = ', 0 /* on instance */'
       if not attribute.is_expose_js_accessors else '' %}
{"{{attribute.name}}", {{getter_callback}}, {{setter_callback}}, {{getter_callback_for_main_world}}, {{setter_callback_for_main_world}}, {{wrapper_type_info}}, {{access_control}}, {{property_attribute}}{{on_prototype}}}
{%- endmacro %}


{##############################################################################}
{% macro method_configuration(method) %}
{% set method_callback =
   '%sV8Internal::%sMethodCallback' % (interface_name, method.name) %}
{% set method_callback_for_main_world =
   '%sV8Internal::%sMethodCallbackForMainWorld' % (interface_name, method.name)
   if method.is_per_world_bindings else '0' %}
{"{{method.name}}", {{method_callback}}, {{method_callback_for_main_world}}, {{method.number_of_required_or_variadic_arguments}}}
{%- endmacro %}


{##############################################################################}
{% block constructor_getter %}
{% if has_constructor_attributes %}
static void {{interface_name}}ConstructorGetter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Value> data = info.Data();
    ASSERT(data->IsExternal());
    V8PerContextData* perContextData = V8PerContextData::from(info.Holder()->CreationContext());
    if (!perContextData)
        return;
    v8SetReturnValue(info, perContextData->constructorForType(WrapperTypeInfo::unwrap(data)));
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block replaceable_attribute_setter_and_callback %}
{% if has_replaceable_attributes or has_constructor_attributes %}
{# FIXME: rename to ForceSetAttributeOnThis, since also used for Constructors #}
static void {{interface_name}}ReplaceableAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    info.This()->ForceSet(name, jsValue);
}

{# FIXME: rename to ForceSetAttributeOnThisCallback, since also used for Constructors #}
static void {{interface_name}}ReplaceableAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    {{interface_name}}V8Internal::{{interface_name}}ReplaceableAttributeSetter(name, jsValue, info);
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block security_check_functions %}
{% if is_check_security and interface_name != 'Window' %}
bool indexedSecurityCheck(v8::Local<v8::Object> host, uint32_t index, v8::AccessType type, v8::Local<v8::Value>)
{
    {{cpp_class}}* imp =  {{v8_class}}::toNative(host);
    return BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError);
}

bool namedSecurityCheck(v8::Local<v8::Object> host, v8::Local<v8::Value> key, v8::AccessType type, v8::Local<v8::Value>)
{
    {{cpp_class}}* imp =  {{v8_class}}::toNative(host);
    return BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError);
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block origin_safe_method_setter %}
{% if has_origin_safe_method_setter %}
static void {{cpp_class}}OriginSafeMethodSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    {# FIXME: don't call GetIsolate 3 times #}
    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain({{v8_class}}::GetTemplate(info.GetIsolate(), worldType(info.GetIsolate())));
    if (holder.IsEmpty())
        return;
    {{cpp_class}}* imp = {{v8_class}}::toNative(holder);
    v8::String::Utf8Value attributeName(name);
    ExceptionState exceptionState(ExceptionState::SetterContext, *attributeName, "{{interface_name}}", info.Holder(), info.GetIsolate());
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), exceptionState)) {
        exceptionState.throwIfNeeded();
        return;
    }

    info.This()->SetHiddenValue(name, jsValue);
}

static void {{cpp_class}}OriginSafeMethodSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    {{cpp_class}}V8Internal::{{cpp_class}}OriginSafeMethodSetter(name, jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block constructor %}
{% if has_constructor %}
{# FIXME: support overloading #}
static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    {% if is_constructor_raises_exception %}
    ExceptionState exceptionState(info.Holder(), info.GetIsolate());
    {% endif %}
    RefPtr<{{cpp_class}}> impl = {{cpp_class}}::create({{constructor_arguments | join(', ')}});
    v8::Handle<v8::Object> wrapper = info.Holder();
    {% if is_constructor_raises_exception %}
    if (exceptionState.throwIfNeeded())
        return;
    {% endif %}

    V8DOMWrapper::associateObjectWithWrapper<{{v8_class}}>(impl.release(), &{{v8_class}}::wrapperTypeInfo, wrapper, info.GetIsolate(), WrapperConfiguration::Dependent);
    v8SetReturnValue(info, wrapper);
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block constructor_callback %}
{% if has_constructor %}
void {{v8_class}}::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::failedToConstruct("{{interface_name}}", "Please use the 'new' operator, this DOM object constructor cannot be called as a function."), info.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    {{cpp_class}}V8Internal::constructor(info);
}

{% endif %}
{% endblock %}

{##############################################################################}
{% block visit_dom_wrapper %}
{% if generate_visit_dom_wrapper_function %}
void {{v8_class}}::visitDOMWrapper(void* object, const v8::Persistent<v8::Object>& wrapper, v8::Isolate* isolate)
{
    {{cpp_class}}* impl = fromInternalPointer(object);
    if (Node* owner = impl->{{generate_visit_dom_wrapper_function}}()) {
        setObjectGroup(V8GCController::opaqueRootForGC(owner, isolate), wrapper, isolate);
        return;
    }
    setObjectGroup(object, wrapper, isolate);
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block class_attributes %}
{# FIXME: rename to install_attributes and put into configure_class_template #}
{% if attributes %}
static const V8DOMConfiguration::AttributeConfiguration {{v8_class}}Attributes[] = {
    {% for attribute in attributes
       if not (attribute.is_expose_js_accessors or
               attribute.is_static or
               attribute.runtime_enabled_function or
               attribute.per_context_enabled_function) %}
    {% filter conditional(attribute.conditional_string) %}
    {{attribute_configuration(attribute)}},
    {% endfilter %}
    {% endfor %}
};

{% endif %}
{% endblock %}


{##############################################################################}
{% block class_accessors %}
{# FIXME: rename install_accessors and put into configure_class_template #}
{% if has_accessors %}
static const V8DOMConfiguration::AccessorConfiguration {{v8_class}}Accessors[] = {
    {% for attribute in attributes if attribute.is_expose_js_accessors %}
    {{attribute_configuration(attribute)}},
    {% endfor %}
};

{% endif %}
{% endblock %}


{##############################################################################}
{% block class_methods %}
{# FIXME: rename to install_methods and put into configure_class_template #}
{% if has_method_configuration %}
static const V8DOMConfiguration::MethodConfiguration {{v8_class}}Methods[] = {
    {% for method in methods if method.do_generate_method_configuration %}
    {% filter conditional(method.conditional_string) %}
    {{method_configuration(method)}},
    {% endfilter %}
    {% endfor %}
};

{% endif %}
{% endblock %}


{##############################################################################}
{% block configure_class_template %}
{# FIXME: rename to install_dom_template and Install{{v8_class}}DOMTemplate #}
static v8::Handle<v8::FunctionTemplate> Configure{{v8_class}}Template(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    {% if runtime_enabled_function %}
    if (!{{runtime_enabled_function}}())
        {# FIXME: support inheritance #}
        defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "", v8::Local<v8::FunctionTemplate>(), {{v8_class}}::internalFieldCount, 0, 0, 0, 0, 0, 0, isolate, currentWorldType);
    else
    {% endif %}
    {% set runtime_enabled_indent = 4 if runtime_enabled_function else 0 %}
    {% filter indent(runtime_enabled_indent, true) %}
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "{{interface_name}}", v8::Local<v8::FunctionTemplate>(), {{v8_class}}::internalFieldCount,
        {# Test needed as size 0 constant arrays are not allowed in VC++ #}
        {% set attributes_name, attributes_length =
               ('%sAttributes' % v8_class,
                'WTF_ARRAY_LENGTH(%sAttributes)' % v8_class)
           if attributes else (0, 0) %}
        {% set accessors_name, accessors_length =
               ('%sAccessors' % v8_class,
                'WTF_ARRAY_LENGTH(%sAccessors)' % v8_class)
           if has_accessors else (0, 0) %}
        {% set methods_name, methods_length =
               ('%sMethods' % v8_class,
                'WTF_ARRAY_LENGTH(%sMethods)' % v8_class)
           if has_method_configuration else (0, 0) %}
        {{attributes_name}}, {{attributes_length}},
        {{accessors_name}}, {{accessors_length}},
        {{methods_name}}, {{methods_length}},
        isolate, currentWorldType);
    {% endfilter %}

    UNUSED_PARAM(defaultSignature);
    {% if has_constructor %}
    functionTemplate->SetCallHandler({{v8_class}}::constructorCallback);
    {# FIXME: compute length #}
    functionTemplate->SetLength(0);
    {% endif %}
    v8::Local<v8::ObjectTemplate> instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> prototypeTemplate = functionTemplate->PrototypeTemplate();
    UNUSED_PARAM(instanceTemplate);
    UNUSED_PARAM(prototypeTemplate);
    {% if is_check_security and interface_name != 'Window' %}
    instanceTemplate->SetAccessCheckCallbacks({{cpp_class}}V8Internal::namedSecurityCheck, {{cpp_class}}V8Internal::indexedSecurityCheck, v8::External::New(isolate, const_cast<WrapperTypeInfo*>(&{{v8_class}}::wrapperTypeInfo)));
    {% endif %}
    {% for attribute in attributes if attribute.runtime_enabled_function %}
    {% filter conditional(attribute.conditional_string) %}
    if ({{attribute.runtime_enabled_function}}()) {
        static const V8DOMConfiguration::AttributeConfiguration attributeConfiguration =\
        {{attribute_configuration(attribute)}};
        V8DOMConfiguration::installAttribute(instanceTemplate, prototypeTemplate, attributeConfiguration, isolate, currentWorldType);
    }
    {% endfilter %}
    {% endfor %}
    {% if constants %}
    {{install_constants() | indent}}
    {% endif %}
    {% for method in methods if not method.do_not_check_signature %}
    {% if method.custom_signature and not method.overload_index %}
    {# No custom signature needed for overloaded methods;
       separate check because depends on global check for overloads #}

    // Custom Signature '{{method.name}}'
    const int {{method.name}}Argc = {{method.arguments | length}};
    v8::Handle<v8::FunctionTemplate> {{method.name}}Argv[{{method.name}}Argc] = { {{method.custom_signature}} };
    v8::Handle<v8::Signature> {{method.name}}Signature = v8::Signature::New(isolate, functionTemplate, {{method.name}}Argc, {{method.name}}Argv);
    {% endif %}
    {# install_custom_signature #}
    {% if not method.overload_index or method.overload_index == 1 %}
    {# For overloaded methods, only generate one accessor #}
    {% filter conditional(method.conditional_string) %}
    {% if method.is_do_not_check_security %}
    {% if method.is_per_world_bindings %}
    if (currentWorldType == MainWorld) {
        {{install_do_not_check_security_signature(method, 'ForMainWorld')}}
    } else {
        {{install_do_not_check_security_signature(method)}}
    }
    {% else %}
    {{install_do_not_check_security_signature(method)}}
    {% endif %}
    {% else %}{# is_do_not_check_security #}
    {% if method.is_per_world_bindings %}
    if (currentWorldType == MainWorld) {
        {% filter runtime_enabled(method.runtime_enabled_function) %}
        {{install_custom_signature(method, 'ForMainWorld')}}
        {% endfilter %}
    } else {
        {% filter runtime_enabled(method.runtime_enabled_function) %}
        {{install_custom_signature(method)}}
        {% endfilter %}
    }
    {% else %}
    {% filter runtime_enabled(method.runtime_enabled_function) %}
    {{install_custom_signature(method)}}
    {% endfilter %}
    {% endif %}
    {% endif %}{# is_do_not_check_security #}
    {% endfilter %}
    {% endif %}{# install_custom_signature #}
    {% endfor %}
    {% for attribute in attributes if attribute.is_static %}
    {% set getter_callback = '%sV8Internal::%sAttributeGetterCallback' %
           (interface_name, attribute.name) %}
    functionTemplate->SetNativeDataProperty(v8::String::NewFromUtf8(isolate, "{{attribute.name}}", v8::String::kInternalizedString), {{getter_callback}}, {{attribute.setter_callback}}, v8::External::New(isolate, 0), static_cast<v8::PropertyAttribute>(v8::None), v8::Handle<v8::AccessorSignature>(), static_cast<v8::AccessControl>(v8::DEFAULT));
    {% endfor %}
    {% if has_custom_legacy_call %}
    functionTemplate->InstanceTemplate()->SetCallAsFunctionHandler({{v8_class}}::legacyCallCustom);
    {% endif %}
    {% if interface_name == 'HTMLAllCollection' %}
    {# Needed for legacy support of document.all #}
    functionTemplate->InstanceTemplate()->MarkAsUndetectable();
    {% endif %}

    // Custom toString template
    functionTemplate->Set(v8::String::NewFromUtf8(isolate, "toString", v8::String::kInternalizedString), V8PerIsolateData::current()->toStringTemplate());
    return functionTemplate;
}

{% endblock %}


{######################################}
{% macro install_do_not_check_security_signature(method, world_suffix) %}
{# FIXME: move to V8DOMConfiguration::installDOMCallbacksWithDoNotCheckSecuritySignature #}
{# Methods that are [DoNotCheckSecurity] are always readable, but if they are
   changed and then accessed from a different origin, we do not return the
   underlying value, but instead return a new copy of the original function.
   This is achieved by storing the changed value as a hidden property. #}
{% set getter_callback =
       '%sV8Internal::%sOriginSafeMethodGetterCallback%s' %
       (cpp_class, method.name, world_suffix) %}
{% set setter_callback =
    '{0}V8Internal::{0}OriginSafeMethodSetterCallback'.format(cpp_class)
    if not method.is_read_only else '0' %}
{% set property_attribute =
    'static_cast<v8::PropertyAttribute>(%s)' %
    ' | '.join(method.property_attributes or ['v8::DontDelete']) %}
{{method.function_template}}->SetAccessor(v8::String::NewFromUtf8(isolate, "{{method.name}}", v8::String::kInternalizedString), {{getter_callback}}, {{setter_callback}}, v8Undefined(), v8::ALL_CAN_READ, {{property_attribute}});
{%- endmacro %}


{######################################}
{% macro install_custom_signature(method, world_suffix) %}
{# FIXME: move to V8DOMConfiguration::installDOMCallbacksWithCustomSignature #}
{% set method_callback = '%sV8Internal::%sMethodCallback%s' %
                         (interface_name, method.name, world_suffix) %}
{% set property_attribute = 'static_cast<v8::PropertyAttribute>(%s)' %
                            ' | '.join(method.property_attributes) %}
{{method.function_template}}->Set(v8::String::NewFromUtf8(isolate, "{{method.name}}", v8::String::kInternalizedString), v8::FunctionTemplate::New(isolate, {{method_callback}}, v8Undefined(), {{method.signature}}, {{method.number_of_required_or_variadic_arguments}}){% if method.property_attributes %}, {{property_attribute}}{% endif %});
{%- endmacro %}


{######################################}
{% macro install_constants() %}
{# FIXME: should use reflected_name instead of name #}
{# Normal (always enabled) constants #}
static const V8DOMConfiguration::ConstantConfiguration {{v8_class}}Constants[] = {
    {% for constant in constants if not constant.runtime_enabled_function %}
    {"{{constant.name}}", {{constant.value}}},
    {% endfor %}
};
V8DOMConfiguration::installConstants(functionTemplate, prototypeTemplate, {{v8_class}}Constants, WTF_ARRAY_LENGTH({{v8_class}}Constants), isolate);
{# Runtime-enabled constants #}
{% for constant in constants if constant.runtime_enabled_function %}
if ({{constant.runtime_enabled_function}}()) {
    static const V8DOMConfiguration::ConstantConfiguration constantConfiguration = {"{{constant.name}}", static_cast<signed int>({{constant.value}})};
    V8DOMConfiguration::installConstants(functionTemplate, prototypeTemplate, &constantConfiguration, 1, isolate);
}
{% endfor %}
{# Check constants #}
{% if not do_not_check_constants %}
{% for constant in constants %}
COMPILE_ASSERT({{constant.value}} == {{cpp_class}}::{{constant.reflected_name}}, TheValueOf{{cpp_class}}_{{constant.reflected_name}}DoesntMatchWithImplementation);
{% endfor %}
{% endif %}
{% endmacro %}


{##############################################################################}
{% block get_template %}
{# FIXME: rename to get_dom_template and GetDOMTemplate #}
v8::Handle<v8::FunctionTemplate> {{v8_class}}::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::EscapableHandleScope handleScope(isolate);
    v8::Local<v8::FunctionTemplate> templ =
        Configure{{v8_class}}Template(data->rawTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Escape(templ);
}

{% endblock %}


{##############################################################################}
{% block has_instance_and_has_instance_in_any_world %}
bool {{v8_class}}::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool {{v8_class}}::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

{% endblock %}


{##############################################################################}
{% block install_per_context_attributes %}
{% if has_per_context_enabled_attributes %}
void {{v8_class}}::installPerContextEnabledProperties(v8::Handle<v8::Object> instanceTemplate, {{cpp_class}}* impl, v8::Isolate* isolate)
{
    v8::Local<v8::Object> prototypeTemplate = v8::Local<v8::Object>::Cast(instanceTemplate->GetPrototype());
    {% for attribute in attributes if attribute.per_context_enabled_function %}
    if ({{attribute.per_context_enabled_function}}(impl->document())) {
        static const V8DOMConfiguration::AttributeConfiguration attributeConfiguration =\
        {{attribute_configuration(attribute)}};
        V8DOMConfiguration::installAttribute(instanceTemplate, prototypeTemplate, attributeConfiguration, isolate);
    }
    {% endfor %}
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block install_per_context_methods %}
{% if has_per_context_enabled_methods %}
void {{v8_class}}::installPerContextEnabledMethods(v8::Handle<v8::Object> prototypeTemplate, v8::Isolate* isolate)
{
    UNUSED_PARAM(prototypeTemplate);
    {# Define per-context enabled operations #}
    v8::Local<v8::Signature> defaultSignature = v8::Signature::New(isolate, GetTemplate(isolate, worldType(isolate)));
    UNUSED_PARAM(defaultSignature);

    ExecutionContext* context = toExecutionContext(prototypeTemplate->CreationContext());
    {% for method in methods if method.per_context_enabled_function %}
    if (context && context->isDocument() && {{method.per_context_enabled_function}}(toDocument(context)))
        prototypeTemplate->Set(v8::String::NewFromUtf8(isolate, "{{method.name}}", v8::String::kInternalizedString), v8::FunctionTemplate::New(isolate, {{cpp_class}}V8Internal::{{method.name}}MethodCallback, v8Undefined(), defaultSignature, {{method.number_of_required_arguments}})->GetFunction());
    {% endfor %}
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block to_active_dom_object %}
{% if is_active_dom_object %}
ActiveDOMObject* {{v8_class}}::toActiveDOMObject(v8::Handle<v8::Object> wrapper)
{
    return toNative(wrapper);
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block wrap %}
{% if special_wrap_for %}
v8::Handle<v8::Object> wrap({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    {% for special_wrap_interface in special_wrap_for %}
    if (impl->is{{special_wrap_interface}}())
        return wrap(to{{special_wrap_interface}}(impl), creationContext, isolate);
    {% endfor %}
    v8::Handle<v8::Object> wrapper = {{v8_class}}::createWrapper(impl, creationContext, isolate);
    return wrapper;
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block create_wrapper %}
{% if not has_custom_to_v8 %}
v8::Handle<v8::Object> {{v8_class}}::createWrapper(PassRefPtr<{{cpp_class}}> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<{{v8_class}}>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::wrapperTypeInfo instead of an XXX::wrapperTypeInfo. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == wrapperTypeInfo.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &wrapperTypeInfo, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextEnabledProperties(wrapper, impl.get(), isolate);
    {% set wrapper_configuration = 'WrapperConfiguration::Dependent'
                                   if (has_visit_dom_wrapper or
                                       is_active_dom_object or
                                       is_dependent_lifetime) else
                                   'WrapperConfiguration::Independent' %}
    V8DOMWrapper::associateObjectWithWrapper<{{v8_class}}>(impl, &wrapperTypeInfo, wrapper, isolate, {{wrapper_configuration}});
    return wrapper;
}

{% endif %}
{% endblock %}


{##############################################################################}
{% block deref_object_and_to_v8_no_inline %}
void {{v8_class}}::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline({{cpp_class}}* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

{% endblock %}
