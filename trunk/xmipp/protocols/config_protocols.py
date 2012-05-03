
#--------------------------------------------------------------------------------
# Protocols info (name, title, path)
#--------------------------------------------------------------------------------
protocols = {
        'import_micrographs': ('Import Micrographs', 'Micrographs/Imported'),
        'downsample_micrographs': ('Downsample Micrographs', 'Micrographs/Downsampled'),
        'screen_micrographs': ('Screen Micrographs', 'Micrographs/Screen'),
        'particle_pick': ('Manual picking', 'ParticlePicking/Manual'),
        'particle_pick_supervised': ('Supervised picking', 'ParticlePicking/Supervised'),
        'particle_pick_auto': ('Automatic picking', 'ParticlePicking/Auto'),
        'extract_particles': ('Extract Particles', 'Images/Extracted'),
        'import_particles': ('Import Particles', 'Images/Imported'),
        'preprocess_particles': ('Preprocess Particles', 'Images/Preprocessed'),
        'screen_particles': ('Screen Particles', 'Images/Screening'),
        'ml2d': ('ML2D', '2D/ML2D'),
        'cl2d': ('CL2D', '2D/CL2D'),
        'cl2d_align': ('Only align', '2D/Alignment'),
        'kerdensom': ('KerDenSOM',  '2D/KerDenSOM'),
        'rotspectra': ('Rotational Spectra', '2D/RotSpectra'),
        'rct': ('Random Conical Tilt', '3D/RCT'),
        'projmatch': ('Projection Matching', '3D/ProjMatch'), 
        'ml3d': ('ML3D', '3D/ML3D'),
        'mltomo': ('MLTomo', '3D/MLTomo'),
        'subtraction': ('Partial Projection Subtraction', '3D/ProjSub'),
        'dummy': ('Dummy', 'Dummy'),
        'xmipp': ('Xmipp Programs', 'XmippPrograms')            
        }

#--------------------------------------------------------------------------------
# Protocols sections and groups
#--------------------------------------------------------------------------------
sections = [
('Preprocessing', 
   [['Micrographs', 'import_micrographs','screen_micrographs','downsample_micrographs'], 
    ['Particle picking', 'particle_pick', 'particle_pick_supervised', 'particle_pick_auto', 'extract_particles'], 
    ['Particles', 'import_particles', 'preprocess_particles', 'screen_particles']]),
('2D', 
   [['Align+Classify', 'ml2d', 'cl2d', 'cl2d_align'], 
    ['Tools', 'kerdensom', 'rotspectra']]),
('3D', 
   [['Initial Model', 'rct'], 
    ['Model Refinement', 'projmatch', 'ml3d']])
,
('Other',
 [['Extra','subtraction', 'mltomo', 'dummy']])
]

#--------------------------------------------------------------------------------
# Protocol data support
#--------------------------------------------------------------------------------
class ProtocolData:
    def __init__(self, section, group, name, title, path):
        self.section = section
        self.group = group
        self.name = name
        self.title = title
        self.dir = path

class ProtocolDictionary(dict):
    def __init__(self):
        for section, sectionList in sections:
            for groupList in sectionList:
                group = groupList[0]
                protocolList = groupList[1:]
                for protocol in protocolList:
                    self.addProtocol(section, group, protocol)
        # Add special 'xmipp_program'
        self.addProtocol(None, None, 'xmipp')

    def addProtocol(self, section, group, protocol):
        title, path = protocols[protocol]
        p = ProtocolData(section, group, protocol, title, path)
        setattr(self, protocol, p)
        self[protocol] = p
        

protDict = ProtocolDictionary()

#--------------------------------------------------------------------------------
# Project default settings
#--------------------------------------------------------------------------------
projectDefaults = {
                   'Cfg': '.project.cfg',
                   'Db': '.project.sqlite',
                   'LogsDir': 'Logs',
                   'RunsDir': 'Runs',
                   'TmpDir': 'Tmp',
                   'RunsPrefix': 'run',
                   'TableGroups': 'groups',
                   'TableParams': 'params',
                   'TableProtocols': 'protocols',
                   'TableProtocolsGroups': 'protocols_groups',
                   'TableRuns': 'runs',
                   'TableSteps': 'steps'
                   } 

#--------------------------------------------------------------------------------
# GUI Properties
#--------------------------------------------------------------------------------

#Font
#FontName = "Helvetica"
FontName = "Verdana"
FontSize = 10

#TextColor
CitationTextColor = "dark olive green"
LabelTextColor = "black"
SectionTextColor = "blue4"

#Background Color
BgColor = "light grey"
LabelBgColor = "white"
HighlightBgColor = BgColor
ButtonBgColor = "LightBlue"
ButtonActiveBgColor = "LightSkyBlue"
EntryBgColor = "lemon chiffon" 
ExpertLabelBgColor = "light salmon"
SectionBgColor = ButtonBgColor

#Color
ListSelectColor = "DeepSkyBlue4"
BooleanSelectColor = "white"
ButtonSelectColor = "DeepSkyBlue2"

#Dimensions limits
MaxHeight = 650
MaxWidth = 800
MaxFontSize = 14
MinFontSize = 6
WrapLenght = MaxWidth - 50