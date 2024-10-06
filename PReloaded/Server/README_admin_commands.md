# Admin commands

https://fodev.net/forum/index.php?topic=29382.0

Command
~getaccess admin YourChosenAdminPassword

`spawnitem pid count -legit
example: `spawnitem 349 1 -legit
`spawnitem 727 1 -legit
`spawnitem 407 1 -legit
`spawnitem 38 5 -a 10 -legit
`spawnitem 39 5 -a 10 -legit

`addnpc 278 -d 10802 -l 1

/******************************************************************
***************       Armor                         ***************
******************************************************************/
//Light
#define PID_LEATHER_JACKET                  (74)
#define PID_LEATHER_ARMOR                   (1)
#define PID_LEATHER_ARMOR_MK_II             (379)
#define PID_CURED_LEATHER_ARMOR             (265)
//Medium
#define PID_METAL_ARMOR                     (2)
#define PID_METAL_ARMOR_MK_II               (380)
#define PID_TESLA_ARMOR                     (240)
#define PID_COMBAT_ARMOR                    (17)
#define PID_COMBAT_ARMOR_MK_II              (381)
#define PID_BROTHERHOOD_COMBAT_ARMOR        (239)
#define PID_NCR_ARMOR                       (9654)
#define PID_ENVIROMENTAL_ARMOR              (751) // Test
#define PID_ENCLAVE_COMBAT_ARMOR            (752)
#define PID_VAULT_13_SUIT                   (753)
#define PID_VAULT_SUIT                      (754)
#define PID_WORN_ROBE                       (755)
#define PID_BLUE_ROBE                       (756)
#define PID_RED_ROBE                        (757)
#define PID_GRAY_ROBE                       (758)
#define PID_MOBSTER_BLUE                    (759)
#define PID_MOBSTER_GRAY                    (760)
#define PID_CLOTHES_POLICE                  (761)
#define PID_CLOTHES_GENERIC1                (762)
#define PID_CLOTHES_GENERIC2                (763)
#define PID_CLOTHES_SCIENTIST1              (764)
#define PID_CLOTHES_SCIENTIST2              (765)
#define PID_CLOTHES_ELEGANT_SUIT            (766)
#define PID_CLOTHES_RAGS                    (767)
#define PID_CLOTHES_DWARF                   (768)
#define PID_CLOTHES_RED_SHIRT               (769)
#define PID_CLOTHES_DOCTOR                  (770)
#define PID_CLOTHES_CHILD                   (771)
#define PID_CLOTHES_GRAY_SHIRT              (772)
#define PID_CLOTHES_FAT1                    (773)
#define PID_CLOTHES_BOXER                   (774)
#define PID_SF_COMBAT_JACKET                (775)
#define PID_MORDINO_METAL_JACKET            (776)
#define PID_CLOTHES_GENERIC3                (777)
#define PID_DESERT_COMBAT_ARMOR             (778)
#define PID_NPC_SKIN                        (799)
//Heavy
#define PID_POWERED_ARMOR                   (3)
#define PID_HARDENED_POWER_ARMOR            (232)
#define PID_ADVANCED_POWER_ARMOR            (348)
#define PID_ADVANCED_POWER_ARMOR_MK2        (349)
//Robes
#define PID_PURPLE_ROBE                     (113)
#define PID_KEEPBRIGE_ROBE                  (524) // PID_BRIDGEKEEPERS_ROBE
#define PID_SAND_ROBE                       (750) // Has same stats as PID_PURPLE_ROBE
//Helmets
#define PID_STRAWHAT                        (719)
#define PID_ARMIN_CAP                       (720)
#define PID_POWER_HELMET                    (721)
#define PID_MOTORCYCLE_HELMET               (722)
//More helmets
#define PID_COMBAT_HELMET                  (725)
#define PID_COMBAT_HELMET_MK_II            (726)
#define PID_BROTHERHOOD_HELMET             (727)
#define PID_LEATHER_JACKET_HELMET          (728)
#define PID_COMBAT_LEATHER_JACKET_HELMET   (729)
#define PID_LEATHER_ARMOR_HELMET           (730)
#define PID_LEATHER_ARMOR_HELMET_MK2       (731)
#define PID_METAL_HELMET                   (732)
#define PID_METAL_HELMET_MK2               (733)
#define PID_TESLA_HELMET                   (734)
#define PID_APA_HELMET                     (735)
#define PID_APA_HELMET_MK2                 (736)
#define PID_HARDENED_POWER_HELMET          (737)
#define PID_ENCLAVE_COMBAT_HELMET          (738)
#define PID_ENCLAVE_COMBAT_HELMET_MK2      (739)
#define PID_DESERT_COMBAT_HELMET           (740)
#define PID_NCR_HELMET                     (741)

/******************************************************************
***************       Weapons                       ***************
******************************************************************/
//Small Guns
    //Pistols
#define PID_ZIP_GUN                         (300)
#define PID_9MM_MAUSER                      (122)
#define PID_10MM_PISTOL                     (8)
#define PID_14MM_PISTOL                     (22)
#define PID_DESERT_EAGLE                    (18)
#define PID_DESERT_EAGLE_EXT_MAG            (404)
#define PID_223_PISTOL                      (241)
#define PID_44_MAGNUM_REVOLVER              (313)
#define PID_44_MAGNUM_SPEEDLOADER           (398)
#define PID_NEEDLER_PISTOL                  (388)  // HN Needler
#define PID_PK12_GAUSS_PISTOL               (394)  // 2mm EC
#define PID_GRENADE_PISTOL					(619)
    //Rifles
#define PID_HUNTING_RIFLE                   (10)
#define PID_SCOPED_HUNTING_RIFLE            (287)
#define PID_SPRINGER_RIFLE                  (299)
#define PID_ASSAULT_RIFLE                   (23)
#define PID_ASSAULT_RIFLE_EXT_MAG           (405)
#define PID_SNIPER_RIFLE                    (143)
#define PID_FN_FAL                          (351)
#define PID_FN_FAL_NIGHT_SCOPE              (403)
#define PID_FN_FAL_HPFA                     (500)
#define PID_RED_RYDER_BB_GUN                (161)
#define PID_RED_RYDER_LE_BB_GUN             (162)
#define PID_JONNY_BB_GUN                    (261)
#define PID_INDEPENDENT                     (353)
#define PID_M72_GAUSS_RIFLE                 (392)  // 2mm EC
#define PID_GRENADE_LAUNCHER				(620)
    //Shotguns
#define PID_SHOTGUN                         (94)
#define PID_SAWED_OFF_SHOTGUN               (385)  // 12 ga.
#define PID_COMBAT_SHOTGUN                  (242)
#define PID_HK_CAWS                         (268)
#define PID_PANCOR_JACKHAMMER               (354)
    //Pistol-machine gun
#define PID_10MM_SMG                        (9)
#define PID_HK_P90C                         (296)
#define PID_TOMMY_GUN                       (283)
#define PID_HK_G11                          (352)
#define PID_HK_G11E                         (391)  // 4.7mm Caseless
#define PID_GREASE_GUN                      (332)
//Big guns
#define PID_FLAMER                          (11)
#define PID_IMPROVED_FLAMETHROWER           (400)
#define PID_ROCKET_LAUNCHER                 (13)
#define PID_MINIGUN                         (12)
#define PID_AVENGER_MINIGUN                 (389)  // 5mm JHP
#define PID_VINDICATOR_MINIGUN              (395)  // 4.7mm Caseless
#define PID_BOZAR                           (350)
#define PID_LIGHT_SUPPORT_WEAPON            (355)
#define PID_M60                             (387)  // 7.62
//Energo
    //Laser
#define PID_LASER_PISTOL                    (16)
#define PID_MAGNETO_LASER_PISTOL            (402)
#define PID_SOLAR_SCORCHER                  (390)  // No ammo
#define PID_LASER_RIFLE                     (118)
#define PID_LASER_RIFLE_EXT_CAP             (401)
#define PID_GATLING_LASER                   (28)
#define PID_ALIEN_LASER_PISTOL              (120)
#define PID_DUAL_LASER_CANNON               (9659) // Vault City Gun Turret
    //Plasma
#define PID_PLASMA_PISTOL                   (24)
#define PID_PLASMA_PISTOL_EXT_CART          (406)
#define PID_PLASMA_RIFLE                    (15)
#define PID_TURBO_PLASMA_RIFLE              (233)
    //Pulse
#define PID_YK32_PULSE_PISTOL               (396)  // Small Energy
#define PID_YK42B_PULSE_RIFLE               (397)  // Micro Fusion
//Throwing
    //Grenade
#define PID_MOLOTOV_COCKTAIL                (159) //Grouped
#define PID_FRAG_GRENADE                    (25) //Grouped
#define PID_PLASMA_GRENADE                  (26) //Grouped
#define PID_PULSE_GRENADE                   (27) //Grouped
//Melee
    //Cutting
#define PID_KNIFE                           (4)
#define PID_COMBAT_KNIFE                    (236)
#define PID_LIL_JESUS_WEAPON                (517)
#define PID_SHIV                            (383)
#define PID_SWITCHBLADE                     (319)
#define PID_WAKIZASHI_BLADE                 (522)
    //Pricking
#define PID_SPEAR                           (7)
#define PID_SHARP_SPEAR                     (280)
#define PID_SHARPENED_POLE                  (320)
    //Shock
#define PID_CLUB                            (5)
#define PID_CROWBAR                         (20)
#define PID_WRENCH                          (384)
#define PID_SLEDGEHAMMER                    (6)
#define PID_LOUISVILLE_SLUGGER              (386)
#define PID_SUPER_SLEDGE                    (115)
    //Electric
#define PID_CATTLE_PROD                     (160)
#define PID_SUPER_CATTLE_PROD               (399)
#define PID_RIPPER                          (116)
//Unarmed
#define PID_BOXING_GLOVES                   (292)
#define PID_PLATED_BOXING_GLOVES            (293)
#define PID_RING_BOXING_GLOVES              (496)
#define PID_RING_PLATED_BOXING_GLOVES       (497)
#define PID_POWER_FIST                      (235)
#define PID_BRASS_KNUCKLES                  (21)
#define PID_SPIKED_KNUCKLES                 (234)
#define PID_MEGA_POWER_FIST                 (407)

//Special
#define PID_ROBO_ROCKET_LAUNCHER            (270)
#define PID_PHAZER                          (393)  // Small Energy
#define PID_DEATHCLAW_CLAW_1                (371)
#define PID_DEATHCLAW_CLAW_2                (372)
#define PID_FIRE_GECKO_FLAME_WEAPON         (427)
#define PID_FLAME_BREATH                    (427)
#define PID_WEAK_FLAME_BREATH               (616)
#define PID_SPECIAL_BOXER_WEAPON            (489)
#define PID_GUN_TURRET_WEAPON               (498)
#define PID_EYEBALL_FIST_1                  (290)
#define PID_EYEBALL_FIST_2                  (291)
#define PID_DUAL_MINIGUN                    (518)
#define PID_HEAVY_DUAL_MINIGUN              (520)
#define PID_END_BOSS_KIFE                   (530)
#define PID_END_BOSS_PLASMA_GUN             (531)
#define PID_HOLY_HAND_GRENADE               (421) // Grenade
#define PID_BAN_HAMMER		                (617) // Mlot do wywalania gracza z gry na "off"
#define PID_HOLY_SPEAR		                (618) // Spear ze specjalnymi nieznanymi dzis efektami (30-08-2013)

/******************************************************************
***************       Ammo                          ***************
******************************************************************/
//Bullets
#define PID_4_7MM_CASELESS                  (359)
#define PID_5MM_JHP                         (35)
#define PID_5MM_AP                          (36)
#define PID_7_62MM_AMMO                     (363)
#define PID_9MM_AMMO                        (360)
#define PID_9MM_BALL                        (121)
#define PID_10MM_JHP                        (29)
#define PID_10MM_AP                         (30)
#define PID_14MM_AP                         (33)
#define PID_44_MAGNUM_JHP                   (31)
#define PID_44_FMJ_MAGNUM                   (111)
#define PID_44_MAGNUM_FMJ           (111) // to be name-consistent
#define PID_45_CALIBER_AMMO                 (357)
#define PID_223_FMJ                         (34)
#define PID_SHOTGUN_DRAGON_BREATH_SHELLS    (850) // 30.03.2014 Cubik: bylo tu kiedys .22 ammo 
#define PID_SHOTGUN_SHELLS                  (95)
#define PID_2MM_EC_AMMO                     (358)
#define PID_SHOTGUN_SHELLS_FLECHETTE        (574)  // from W2155
#define PID_SHOTGUN_SHELLS_SLUG             (575)  // from W2155
#define PID_SHOTGUN_SHELLS_EMP              (576)  // from W2155
#define PID_40MM_GRENADE					(621)
//Other
#define PID_EXPLOSIVE_ROCKET                (14)
#define PID_ROCKET_AP                       (37)
#define PID_FLAMETHROWER_FUEL               (32)
#define PID_FLAMETHROWER_FUEL_MK_II         (382)
#define PID_SMALL_ENERGY_CELL               (38)
#define PID_SEC                             (38) // alias
#define PID_MICRO_FUSION_CELL               (39)
#define PID_MFC                             (39) // alias
#define PID_BBS                             (163)
#define PID_HN_NEEDLER_CARTRIDGE            (361)
#define PID_HN_AP_NEEDLER_CARTRIDGE         (362)
//Special
#define PID_ROBO_ROCKET_AMMO                (274)

/******************************************************************
***************       Drugs                         ***************
******************************************************************/
//Preparations
#define PID_STIMPAK                         (40)
#define PID_RADAWAY                         (48)
#define PID_ANTIDOTE                        (49)
#define PID_RAD_X                           (109)
#define PID_SUPER_STIMPAK                   (144)
#define PID_JET_ANTIDOTE                    (260)
#define PID_HEALING_POWDER                  (273)
#define PID_HYPO                            (525)
#define PID_WEAK_HEALING_POWDER             (9655)
//Alcohol
#define PID_NUKA_COLA                       (106)
#define PID_BEER                            (124)
#define PID_BOOZE                           (125)
#define PID_GAMMA_GULP_BEER                 (310)
#define PID_ROENTGEN_RUM                    (311)
#define PID_ROT_GUT                         (469)
//Drug
#define PID_MENTATS                         (53)
#define PID_BUFFOUT                         (87)
#define PID_PSYCHO                          (110)
#define PID_JET                             (259)
//Other
#define PID_MUTATED_FRUIT                   (71)
#define PID_IGUANA_ON_A_STICK               (81)
#define PID_MEAT_ON_A_STICK                 (103)
#define PID_COOKIE                          (378)
//Special
#define PID_HYPO_POISON                     (334)
#define PID_MUTATED_TOE                     (473)
#define PID_KITTY_SEX_DRUG_AGILITY          (480)  // + 1 agility for 1 hr
#define PID_KITTY_SEX_DRUG_INTELLIGENCE     (481)  // + 1 iq for 1 hr
#define PID_KITTY_SEX_DRUG_STRENGTH         (482)  // + 1 strength for 1 hr
#define PID_MONUMENT_CHUNCK                 (424)

//Tools
    //Tech
#define PID_MULTI_TOOL                      (75)  // +25 repair
#define PID_SUPER_TOOL_KIT                  (308) // +50 repair
#define PID_OIL_CAN                         (412) // 100% repair
#define PID_LOCKPICKS                       (84)  // +25 lockpick simple
#define PID_EXP_LOCKPICK_SET                (410) // +50 lockpick simple
#define PID_ELECTRONIC_LOCKPICKS            (77)  // +25 lockpick electronic
#define PID_ELEC_LOCKPICK_MKII              (411) // +50 lockpick electronic
#define PID_LOCK                            (549)
    //Medical
#define PID_FIRST_AID_KIT                   (47)  // +25 first aid
#define PID_FIELD_MEDIC_KIT                 (408) // +50 first aid
#define PID_DOCTORS_BAG                     (91)  // +25 doctor
#define PID_PARAMEDICS_BAG                  (409) // +50 doctor
#define PID_MEDICAL_SUPPLIES                (428)
#define PID_BIO_GEL                         (440)
    //Other
#define PID_SHOVEL                          (289) //Ëîïàòà
#define PID_PLANK                           (297) //Ëîì
#define PID_AXE                             (543) //Òîïîð
    //Special
#define PID_GEIGER_COUNTER                  (52)
#define PID_ACTIVE_GEIGER_COUNTER           (207) //Active
#define PID_STEALTH_BOY                     (54)
#define PID_ACTIVE_STEALTH_BOY              (210) //Active
#define PID_MOTION_SENSOR                   (59)
#define PID_ACTIVE_MOTION_SENSOR            (208) //Active
#define PID_WATER_BAG                       (605)

//Books
#define PID_BIG_BOOK_OF_SCIENCE             (73)
#define PID_DEANS_ELECTRONICS               (76)
#define PID_FIRST_AID_BOOK                  (80)
#define PID_SCOUT_HANDBOOK                  (86)
#define PID_GUNS_AND_BULLETS                (102)
#define PID_CATS_PAW                        (225)
#define PID_TECHNICAL_MANUAL                (228)
#define PID_CHEMISTRY_MANUAL                (237)
#define PID_CATS_PAW_ISSUE_5                (331)
#define PID_BECKY_BOOK                      (471)
#define PID_BARTER_BOOK						(22050)

1. exit
Level the access: any.
Output from the game.

2. myinfo
Level the access: any.
Demand mapping a certain information about the current character - identifier, name, password, the level access.

3. gameinfo <type>
Level the access: moderator.
The demand mapping information about the game, where the type the inquired information (type) can be the following:
0) the statistics the use memory;
1) summary according to the current players in the game;
2) the statistics locations and maps;
3) the current temporary events;
4) the current state data anyData;
5) the statistics objects in the game;

4. id <name>
Level the access: moderator.
Demand the identifier [krittera] on its name (name).
Note:
It is not active at the given moment.

5. move <id> <hx> <hy>
Level the access: moderator.
To move character (id) into specific [geks] (hx, hy).

6. kill <id>
Level the access: moderator.
To kill character (id).

7. disconnect <id>
Level the access: moderator.
To disconnect character (id) from the server.

8. toglobal
Level the access: tester.
To leave by the current character on the global map.

9. respawn <id>
Level the access: moderator.
To resurrect character (id). If id it is equal to 0, then it means to resurrect the current character.

10. param <type> <num> <value>
Level the access: tester.
To change the parameter in the current character.
type must be equal to zero, num to the number the parameter, value to its new value.

11. getaccess <access name> <access password>
Level the access: any.
To change the level access.
access name - name access (client, tester, moder, admin).
access password - password access.

12. crash
Level the access: implementor.
To drop [servernoe] application.

13. additem <hx> <hy> <pid> <count>
Level the access: moderator.
To add object (pid, count) to specific [geks] (hx, hy).

14. additemself, ais <pid> <count>
Level the access: moderator.
To add object (pid, count) into the inventory.

15. addnpc <hx> <hy> <dir> <pid>
Level the access: moderator.
To add [Npts] (pid, dir) to specific [geks] (hx, hy).

16. addloc <wx> <wy> <pid>
Level the access: moderator.
To add location (pid) into the fixed point (wx, wy) on the global map.

17. reloadscripts
Level the access: administrator.
To [perezagruzit] all [servernye] scripts.

18. loadscript, load <script name>
Level the access: administrator.
To load or to [perezagruzit] the specific [servernyy] script (script name).

19. reloadclientscripts, rcs
Level the access: administrator.
To [perezagruzit] client scripts.

20. runscript, run <module name> <func name> <param0> <param1> <param2>
Level the access: administrator.
To neglect the special [servernuyu] function (module name, func name), with the specific parameters (param0, param1, param2).

21. reloadlocations
Level the access: administrator.
To [perezagruzit] the prototypes locations.

22. loadlocation <pid>
Level the access: administrator.
To load the specific prototype (pid) location.

23. reloadmaps
Level the access: administrator.
To [reload] the prototypes maps.

24. loadmap <pid>
Level the access: administrator.
To load the specific prototype (pid) map.

25. regenmap
Level the access: administrator.
To refresh map on which at the current moment is situated character. THIS IS A USEFUL COMMAND TO FIX ANY ACCIDENT WITH THE MAP.
Note:
On the given moment it works with the failures.

26. reloaddialogs
Level the access: administrator.
To [perezagruzit] all dialogues.

27. loaddialog <dialog name> <dialog id>
Level the access: administrator.
To load the specific dialogue (dialog name), under the specific identifier (dialog id).

28. reloadtexts
Level the access: administrator.
To [perezagruzit] all MSG files.

29. reloadai
Level the access: administrator.
To [perezagruzit] the bag [npts] (Bags.cfg).

30. checkvar, cvar <tid var> <master is npc> <master id> <slave id> <full_info>
Level the access: moderator.
To verify the state the specific play variable.
tid var - number variable;
master is npc - does appear master id from [Npts], 0 are not, rest - yes. If yes, then to master id automatically is added 5kk, t to the indexing [npts] begins from this number;
master id - primary identifier;
slave id - second identifier;
full_info - to derive complete information in terms the variable, 0 are not, rest - yes.

31. setvar, svar <tid var> <master is npc> <master id> <slave id> <value>
Level the access: moderator.
To verify the state the specific play variable.
tid var - number variable;
master is npc - does appear master id from [Npts], 0 are not, rest - yes. If yes, then to master id automatically is added 5kk, t to the indexing [npts] begins from this number;
master id - primary identifier;
slave id - second identifier;
value - new value variable.

32. settime <tmul> <year> <month> <day> <hour> <minute> <second>
Level the access: administrator.
To change play time.
tmul - the acceleration time, 0 - not to change;
year - year, 0 - not to change;
month - month, 0 - not to change;
day - day, 0 - not to change;
hour - hour;
minute - minute;
second - second;

33. ban <add, add+, delete, list> <user> <hours> <comment>
Level the access: moderator.
To [zabanit] the specific player.
add - to add into [ban] character;
add+ - to add into [ban] character and IP address;
delete - to remove from the list [zabanennykh];
list - to derive the list [zabanennykh];
user - name character;
hours - time blocking, in the hours;
comment - commentary, it is possible not to write.

34. deleteself <password>
Level the access: any.
To remove the current character.

35. changepassword, changepass <password> <new password>
Level of the access: any.
To change password for the current character.

36. dropuid, drop
Level the access: tester.
To drop anti-alto timer for this character, that it was possible to [perezayti] for another character without waiting the specific time.
