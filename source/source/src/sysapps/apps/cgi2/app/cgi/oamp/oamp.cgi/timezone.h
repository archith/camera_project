struct tz_ds
{
	char* timeZoneName;
	char* POSIXTimeZoneId;
	int tz_hour;
	int tz_min;
	int daylight;
};
struct tz_ds timezone_table[]=
{
	{
		"(GMT-12:00) International Date Line West",
		"IDLE",
		-12,
		0,
		0,
	},
	{
		"(GMT-11:00) Midway Island, Samoa",
		"SST",
		-11,
		0,
		0,
	},
	{
		"(GMT-10:00) Hawaii",
		"HST",
		-10,
		0,
		0,
	},
	{
		"(GMT-09:00) Alaska",
		"AKST",
		-9,
		0,
		1,		
	},
	{
		"(GMT-08:00) Pacific Time (US & Canada); Tijuana",
		"PST",
		-8,
		0,
		1,
	},
	{
		"(GMT-07:00) Arizona",
		"MST",
		-7,
		0,
		0,
	},
	{
		"(GMT-07:00) Chihuahua, La Paz, Mazatlan",
		"",
		-7,
		0,
		1,
	},
	{
		"(GMT-07:00) Mountain Time (US & Canada)",
		"MST",
		-7,
		0,
		1,
	},
	{
		"(GMT-06:00) Central America",
		"CST",
		-6,
		0,
		0,
	},
	{
		"(GMT-06:00) Central Time (US & Canada)",
		"CST",
		-6,
		0,
		1,
	},
	{
		"(GMT-06:00) Guadalajara, Mexico City,Monterrey",
		"CDT",
		-6,
		0,
		1,
	},
	{
		"(GMT-06:00) Saskatchewan",
		"",
		-6,
		0,
		0,		
	},
	{
		"(GMT-05:00) Bogota, Lima, Quito",
		"COT",
		-5,
		0,
		0,
	},
	{
		"(GMT-05:00) Eastern Time (US & Canada)",
		"EST",
		-5,
		0,
		1,
	},
	{
		"(GMT-05:00) Indiana (East)",
		"EST",
		-5,
		0,
		0,
	},
	{
		"(GMT-04:00) Atlantic Time (Canada)",
		"AST",
		-4,
		0,
		1,
	},
	{
		"(GMT-04:00) Caracas, La Paz",
		"VET",
		-4,
		0,
		0,
	},
	{
		"(GMT-04:00) Santiago",
		"CLT",
		-4,
		0,
		1,
	},
	{
		"(GMT-03:30) Newfoundland",
		"NFT",
		-3,
		-30,
		1,
	},
	{
		"(GMT-03:00) Brasilia",
		"BRT",
		-3,
		0,
		1,
	},
	{
		"(GMT-03:00) Buenos Aires, Georgetown",
		"ART",
		-3,
		0,
		0,
	},
	{
		"(GMT-03:00) Greenland",
		"",
		-3,
		0,
		1,
	},
	{
		"(GMT-02:00) Mid-Atlantic",
		"",
		-2,
		0,
		1,
	},
	{
		"(GMT-01:00) Azores",
		"AZOST",
		-1,
		0,
		1,
	},
	{
		"(GMT-01:00) Cape Verde Is.",
		"CVT",
		-1,
		0,
		0,
	},
	{
		"(GMT) Casablanca, Monrovia",
		"WET",
		0,
		0,
		0,
	},
	{
		"(GMT) Greenwich Mean Time: Dublin,Edinburgh,Lisbon,London",
		"BST",
		0,
		0,
		1,
	},
	{
		"(GMT+01:00) Amsterdam, Berlin,Bern,Rome,Stockholm,Vienna",
		"CEST",
		1,
		0,
		1,
	},
	{
		"(GMT+01:00) Belgrade, Bratislava,Budapest,Ljubljana,Prague",
		"CEST",
		1,
		0,
		1,
	},
	{
		"(GMT+01:00) Brussels, Copenhagen, Madrid, Paris",
		"CEST",
		1,
		0,
		1,
	},
	{
		"(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb",
		"CEST",
		1,
		0,
		1,
	},
	{
		"(GMT+01:00) West Central Africa",
		"",
		1,
		0,
		0,
	},
	{
		"(GMT+02:00) Athens, Istanbul, Minsk",
		"EEST",
		2,
		0,
		1,
	},
	{
		"(GMT+02:00) Bucharest",
		"",
		2,
		0,
		1,		
	},
	{
		"(GMT+02:00) Cairo",
		"EEST",
		2,
		0,
		1,
	},
	{
		"(GMT+02:00) Harare, Pretoria",
		"CAT",
		2,
		0,
		0,
	},
	{
		"(GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius",
		"EEST",
		2,
		0,
		1,
	},
	{
		"(GMT+02:00) Jerusalem",
		"IDT",
		2,
		0,
		0,
	},
	{
		"(GMT+03:00) Baghdad",
		"ADT",
		3,
		0,
		1,
	},
	{
		"(GMT+03:00) Kuwait, Riyadh",
		"AST",
		3,
		0,
		0,
	},
	{
		"(GMT+03:00) Moscow, St. Petersburg, Volgograd",
		"MSD",
		3,
		0,
		1,
	},
	{
		"(GMT+03:00) Nairobi",
		"EAT",
		3,
		0,
		0,
	},
	{
		"(GMT+03:30) Tehran",
		"IRDT",
		3,
		30,
		1,
	},
	{
		"(GMT+04:00) Abu Dhabi, Muscat",
		"",
		4,
		0,
		0,
	},
	{
		"(GMT+04:00) Baku, Tbilisi, Yerevan",
		"AZST",
		4,
		0,
		1,
	},
	{
		"(GMT+04:30) Kabul",
		"AFT",
		4,
		30,
		0,
	},
	{
		"(GMT+05:00) Ekaterinburg",
		"",
		5,
		0,
		1,
	},
	{
		"(GMT+05:00) Islamabad, Karachi, Tashkent",
		"",
		5,
		0,
		0,
	},
	{
		"(GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi",
		"PKT",
		5,
		30,
		0,
	},
	{
		"(GMT+05:45) Kathmandu",
		"",
		5,
		45,
		0,
	},
	{
		"(GMT+06:00) Almaty, Novosibirsk",
		"ALMT",
		6,
		0,
	},
	{
		"(GMT+06:00) Astana, Dhaka",
		"BDT",
		6,
		0,
		1,
	},
	{
		"(GMT+06:00) Sri Jayawardenepura",
		"",
		6,
		0,
		0,
	},
	{
		"(GMT+06:30) Rangoon",
		"MMT",
		6,
		30,
		0,
	},
	{
		"(GMT+07:00) Bangkok, Hanoi, Jakarta",
		"ICT",
		7,
		0,
		0,
	},
	{
		"(GMT+07:00) Krasnoyarsk",
		"KRAST",
		7,
		0,
		1,
	},
	{
		"(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi",
		"CST",
		8,
		0,
		0,
	},
	{
		"(GMT+08:00) Irkutsk, Ulaan Bataar",
		"IRKST",
		8,
		0,
		1,
	},
	{
		"(GMT+08:00) Kuala Lumpur, Singapore",
		"MYT",
		8,
		0,
		0,
	},
	{
		"(GMT+08:00) Perth",
		"WST",
		8,
		0,
		0,
	},
	{
		"(GMT+08:00) Taipei",
		"CST",
		8,
		0,
		0,
	},
	{
		"(GMT+09:00) Osaka, Sapporo, Tokyo",
		"JST",
		9,
		0,
		0,
	},
	{
		"(GMT+09:00) Seoul",
		"KST",
		9,
		0,
		0,
	},
	{
		"(GMT+09:00) Yakutsk",
		"YAKST",
		9,
		0,
		1,
	},
	{
		"(GMT+09:30) Adelaide",
		"CST",
		9,
		30,
		1,
	},
	{
		"(GMT+09:30) Darwin",
		"CST",
		9,
		30,
		0,
	},
	{
		"(GMT+10:00) Brisbane",
		"EST",
		10,
		0,
		0,
	},
	{
		"(GMT+10:00) Canberra, Melbourne, Sydney",
		"EST",
		10,
		0,
		1,
	},
	{
		"(GMT+10:00) Guam, Port Moresby",
		"ChST",
		10,
		0,
		0,
	},
	{
		"(GMT+10:00) Hobart",
		"EST",
		10,
		0,
		1,
	},
	{
		"(GMT+10:00) Vladivos",
		"VLAST",
		10,
		0,
		1,
	},
	{
		"(GMT+11:00) Magadan, Solomon Is., New Caledonia",
		"MAGST",
		11,
		0,
		0,
	},
	{
		"(GMT+12:00) Auckland, Wellington",
		"NZST",
		12,
		0,
		1,
	},
	{
		"(GMT+12:00) Fiji, Kamchatka, Marshall Is.",
		"FJT",
		12,
		0,
		0,
	},
	{
		"(GMT+13:00) Nuku'alofa",
		"",
		13,
		0,
		0,
	},
	{
		NULL,
		NULL,
		0,
		0
	},
};
