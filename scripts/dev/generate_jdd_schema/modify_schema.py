def anyOf():
    return [
        {
            "type": "number",
        },
        {
            "type": "string"
        }
    ]


def isInt(s):
    try:
        int(s)
        return True
    except ValueError:
        return False

extension_renaming = {
    'LifeCycleCost:UseAdjustment': 'multipliers',
    'LifeCycleCost:UsePriceEscalation': 'escalations',
    'ElectricLoadCenter:Transformer': 'meters',
    'ElectricLoadCenter:Generators': 'generator_outputs',
    'Generator:FuelCell:AirSupply': 'constituent_fractions',
    'DemandManager:ExteriorLights': 'lights',
    'DemandManager:Ventilation': 'controllers',
    'DemandManagerAssignmentList': 'manager_data',
    'DemandManager:Lights': 'lights',
    'DemandManager:Thermostats': 'thermostats',
    'DemandManager:ElectricEquipment': 'equipment',
    'DaylightingDevice:Tubular': 'transition_lengths',
    'Daylighting:Controls': 'control_data',
    'ZoneHVAC:Baseboard:RadiantConvective:Steam': 'surface_fractions',
    'ZoneHVAC:Baseboard:RadiantConvective:Electric': 'surface_fractions',
    'ZoneHVAC:HighTemperatureRadiant': 'surface_fractions',
    'ZoneHVAC:LowTemperatureRadiant:SurfaceGroup': 'surface_fractions',
    'ZoneHVAC:Baseboard:RadiantConvective:Water': 'surface_fractions',
    'ZoneHVAC:VentilatedSlab:SlabGroup': 'data',
    'ZoneHVAC:CoolingPanel:RadiantConvective:Water': 'surface_fractions',
    'AirConditioner:VariableRefrigerantFlow:FluidTemperatureControl:HR': 'loading_indices',
    'AirConditioner:VariableRefrigerantFlow:FluidTemperatureControl': 'loading_indices',
    'ZoneTerminalUnitList': 'terminal_units',
    'RoomAir:TemperaturePattern:NondimensionalHeight': 'pairs',
    'RoomAir:Node:AirflowNetwork:InternalGains': 'gains',
    'RoomAirSettings:AirflowNetwork': 'nodes',
    'RoomAir:Node:AirflowNetwork:HVACEquipment': 'equipment_fractions',
    'RoomAir:TemperaturePattern:SurfaceMapping': 'surface_deltas',
    'RoomAir:Node:AirflowNetwork:AdjacentSurfaceList': 'surfaces',
    'Controller:MechanicalVentilation': 'zone_specifications',
    'WaterUse:Connections': 'connections',
    'WaterUse:RainCollector': 'surfaces',
    'Output:Table:Annual': 'variable_details',
    'Meter:CustomDecrement': 'variable_details',
    'Output:Table:Monthly': 'variable_details',
    'Output:Table:SummaryReports': 'reports',
    'Meter:Custom': 'variable_details',
    'UnitarySystemPerformance:Multispeed': 'flow_ratios',
    'SurfaceProperty:ExteriorNaturalVentedCavity': 'surface',
    'ZoneProperty:UserViewFactors:bySurfaceName': 'view_factors',
    'SurfaceProperty:HeatTransferAlgorithm:SurfaceList': 'surface',
    'AirLoopHVAC:ZoneSplitter': 'nodes',
    'AirLoopHVAC:SupplyPath': 'components',
    'AirLoopHVAC:ReturnPath': 'components',
    'AirLoopHVAC:ReturnPlenum': 'nodes',
    'AirLoopHVAC:ZoneMixer': 'nodes',
    'AirLoopHVAC:SupplyPlenum': 'nodes',
    'BuildingSurface:Detailed': 'vertices',
    'Shading:Zone:Detailed': 'vertices',
    'RoofCeiling:Detailed': 'vertices',
    'Shading:Site:Detailed': 'vertices',
    'Wall:Detailed': 'vertices',
    'ZoneList': 'zones',
    'Floor:Detailed': 'vertices',
    'Shading:Building:Detailed': 'vertices',
    'SolarCollector:UnglazedTranspired:Multisystem': 'systems',
    'SolarCollector:UnglazedTranspired': 'surfaces',
    'Parametric:SetValueForRun': 'values',
    'Parametric:Logic': 'lines',
    'Parametric:FileNameSuffix': 'suffixes',
    'Parametric:RunControl': 'runs',
    'ZoneHVAC:EquipmentList': 'equipment',
    'AvailabilityManagerAssignmentList': 'managers',
    'Table:MultiVariableLookup': 'values',
    'Table:OneIndependentVariable': 'values',
    'Table:TwoIndependentVariables': 'values',
    'Matrix:TwoDimension': 'values',
    'WindowMaterial:GlazingGroup:Thermochromic': 'temperature_data',
    'Schedule:Compact': 'data',
    'Schedule:Day:Interval': 'data',
    'Schedule:Week:Compact': 'data',
    'EnergyManagementSystem:GlobalVariable': 'variables',
    'EnergyManagementSystem:ProgramCallingManager': 'programs',
    'EnergyManagementSystem:Program': 'lines',
    'EnergyManagementSystem:Subroutine': 'lines',
    'Refrigeration:CaseAndWalkInList': 'cases_and_walkins',
    'Refrigeration:CompressorList': 'compressors',
    'ZoneHVAC:RefrigerationChillerSet': 'chillers',
    'Refrigeration:WalkIn': 'zone_data',
    'Refrigeration:TransferLoadList': 'transfer_loads',
    'Branch': 'components',
    'PipingSystem:Underground:Domain': 'pipe_circuits',
    'Connector:Splitter': 'branches',
    'Connector:Mixer': 'branches',
    'BranchList': 'branches',
    'PipingSystem:Underground:PipeCircuit': 'pipe_segments',
    'NodeList': 'nodes',
    'OutdoorAir:NodeList': 'nodes'
}

def change_version(schema):
    schema["jdd_version"] = "${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}.${CMAKE_VERSION_PATCH}"
    schema["jdd_build"] = "${CMAKE_VERSION_BUILD}"
    loc = schema['properties']['Version']['patternProperties']['.*']['properties']['version_identifier']
    loc['default'] = "${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}"
    loc['type'] = "string"


def change_schedule_compact(schema):
    loc = schema['properties']['Schedule:Compact']['patternProperties']['.*']['properties']['extensions']['items']['properties']['field']
    loc.pop('type')
    loc['anyOf'] = anyOf()


def change_special_cased_enums(schema):
    loc = schema['properties']['GroundHeatTransfer:Slab:Insulation']['patternProperties']['.*']['properties']['ivins_flag_is_there_vertical_insulation']
    loc.pop('type')
    newAnyOf = anyOf()
    newAnyOf[0]['enum'] = [int(i) for i in loc['enum'][:] if isInt(i)]
    newAnyOf[1]['enum'] = ['']
    loc['anyOf'] = newAnyOf
    loc.pop('enum')

    loc = schema['properties']['WindowMaterial:Screen']['patternProperties']['.*']['properties']['angle_of_resolution_for_screen_transmittance_output_map']
    loc.pop('type')
    newAnyOf = anyOf()
    newAnyOf[0]['enum'] = [int(i) for i in loc['enum'][:] if isInt(i)]
    newAnyOf[1]['enum'] = ['']
    loc['anyOf'] = newAnyOf
    loc.pop('enum')

    loc = schema['properties']['Refrigeration:System']['patternProperties']['.*']['properties']['number_of_compressor_stages']
    loc.pop('type')
    newAnyOf = anyOf()
    newAnyOf[0]['enum'] = [int(i) for i in loc['enum'][:] if isInt(i)]
    newAnyOf[1]['enum'] = ['']
    loc['anyOf'] = newAnyOf
    loc.pop('enum')

    loc = schema['properties']['ElectricLoadCenter:Transformer']['patternProperties']['.*']['properties']['phase']
    loc.pop('type')
    newAnyOf = anyOf()
    newAnyOf[0]['enum'] = [int(i) for i in loc['enum'][:] if isInt(i)]
    newAnyOf[1]['enum'] = ['']
    loc['anyOf'] = newAnyOf
    loc.pop('enum')

    loc = schema['properties']['Zone']['patternProperties']['.*']['properties']['zone_outside_convection_algorithm']['enum']
    loc.insert(0, '')

    loc = schema['properties']['Zone']['patternProperties']['.*']['properties']['zone_inside_convection_algorithm']['enum']
    loc.insert(0, '')


def change_utility_cost(schema):
    loc = schema['properties']['UtilityCost:Charge:Block']
    legacy_idd = loc['legacy_idd']['fields']
    loc = loc['patternProperties']['.*']['properties']
    for i in range(6, len(legacy_idd)):
        field = legacy_idd[i]
        loc[field].pop('type')
        loc[field]['anyOf'] = anyOf()

    loc = schema['properties']['UtilityCost:Ratchet']['patternProperties']['.*']['properties']
    loc['offset_value_or_variable_name'].pop('type')
    loc['offset_value_or_variable_name']['anyOf'] = anyOf()
    loc['multiplier_value_or_variable_name'].pop('type')
    loc['multiplier_value_or_variable_name']['anyOf'] = anyOf()

    loc = schema['properties']['UtilityCost:Charge:Simple']['patternProperties']['.*']['properties']
    loc['cost_per_unit_value_or_variable_name'].pop('type')
    loc['cost_per_unit_value_or_variable_name']['anyOf'] = anyOf()

    loc = schema['properties']['UtilityCost:Qualify']['patternProperties']['.*']['properties']
    loc['threshold_value_or_variable_name'].pop('type')
    loc['threshold_value_or_variable_name']['anyOf'] = anyOf()

    loc = schema['properties']['UtilityCost:Tariff']['patternProperties']['.*']['properties']
    loc['minimum_monthly_charge_or_variable_name'].pop('type')
    loc['minimum_monthly_charge_or_variable_name']['anyOf'] = anyOf()
    loc['monthly_charge_or_variable_name'].pop('type')
    loc['monthly_charge_or_variable_name']['anyOf'] = anyOf()


def change_special_cased_name_fields(schema):
    schema['properties']['ZoneHVAC:TerminalUnit:VariableRefrigerantFlow']['legacy_idd']['fields'][0] = 'name'
    schema['properties']['ZoneHVAC:TerminalUnit:VariableRefrigerantFlow']['legacy_idd']['alphas']['fields'][0] = 'name'
    del schema['properties']['ZoneHVAC:TerminalUnit:VariableRefrigerantFlow']['patternProperties']['.*']['required'][0]
    schema['properties']['ZoneHVAC:TerminalUnit:VariableRefrigerantFlow']['name'] = \
         schema['properties']['ZoneHVAC:TerminalUnit:VariableRefrigerantFlow']['patternProperties']['.*']['properties'].pop('zone_terminal_unit_name')

    schema['properties']['AirConditioner:VariableRefrigerantFlow']['legacy_idd']['fields'][0] = 'name'
    schema['properties']['AirConditioner:VariableRefrigerantFlow']['legacy_idd']['alphas']['fields'][0] = 'name'
    del schema['properties']['AirConditioner:VariableRefrigerantFlow']['patternProperties']['.*']['required'][0]
    schema['properties']['AirConditioner:VariableRefrigerantFlow']['name'] = \
        schema['properties']['AirConditioner:VariableRefrigerantFlow']['patternProperties']['.*']['properties'].pop('heat_pump_name')


def change_extensions_name(schema):
    for key, value in extension_renaming.items():
        schema['properties'][key]['patternProperties']['.*']['properties'][value] = schema['properties'][key]['patternProperties']['.*']['properties']['extensions']
        loc = schema['properties'][key]['patternProperties']['.*']['properties']
        del loc['extensions']
        schema['properties'][key]['legacy_idd']['extension'] = value
