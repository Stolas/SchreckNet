#!/usr/bin/env python
#
# Copyright (c) 2024, SchreckNet Authors
#
# SPDX-License-Identifier: BSD-3-Clause
##

import requests
import json
from datetime import datetime

def fetch_jobj():
    KRCG_URL = "https://static.krcg.org/data/vtes.json"
    resp = requests.get(KRCG_URL)
    print(f"Status Code: {resp.status_code}")
    return resp.json()

def get_short_name(name):
    nameLookupDict = {}
    nameLookupDict["Jyhad"] = "Jyhad"
    nameLookupDict["Vampire: The Eternal Struggle"] = "V:TES"
    nameLookupDict["Dark Sovereigns"] = "DS"
    nameLookupDict["Ancient Hearts"] = "AH"
    nameLookupDict["Sabbat"] = "Sabbat"
    nameLookupDict["Sabbat War"] = "SW"
    nameLookupDict["Final Nights"] = "FN"
    nameLookupDict["Bloodlines"] = "BL"
    nameLookupDict["Camarilla Edition"] = "CE"
    nameLookupDict["Anarchs"] = "Anarchs"
    nameLookupDict["Black Hand"] = "BH"
    nameLookupDict["Gehenna"] = "Gehenna"
    nameLookupDict["Tenth Anniversary"] = "Tenth"
    nameLookupDict["Kindred Most Wanted"] = "KMW"
    nameLookupDict["Legacies of Blood"] = "LoB"
    nameLookupDict["Nights of Reckoning"] = "NoR"
    nameLookupDict["Third Edition"] = "Third"
    nameLookupDict["Sword of Caine"] = "SoC"
    nameLookupDict["Lords of the Night"] = "LotN"
    nameLookupDict["Blood Shadowed Court"] = "BSC"
    nameLookupDict["Twilight Rebellion"] = "TR"
    nameLookupDict["Keepers of Tradition"] = "KoT"
    nameLookupDict["Ebony Kingdom"] = "EK"
    nameLookupDict["Heirs to the Blood"] = "HttB"
    nameLookupDict["Danse Macabre"] = "DM"
    nameLookupDict["The Unaligned"] = "TA"
    nameLookupDict["Anarch Unbound"] = "AU"
    nameLookupDict["Lost Kindred"] = "LK"
    nameLookupDict["Sabbat Preconstructed"] = "SP"
    nameLookupDict["Fifth Edition"] = "V5"
    nameLookupDict["Fifth Edition (Anarch)"] = "V5A"
    nameLookupDict["Shadows of Berlin"] = "SoB"
    nameLookupDict["New Blood"] = "NB"
    nameLookupDict["Fall of London"] = "FoL"
    nameLookupDict["Anthology"] = "Ath"
    nameLookupDict["New Blood II"] = "NB2"
    nameLookupDict["Echoes of Gehenna"] = "EoG"
    nameLookupDict["Keepers of Tradition Reprint"] = "KoTR"
    nameLookupDict["Heirs to the Blood Reprint"] = "HttBR"
    nameLookupDict["First Blood"] = "1e"
    nameLookupDict["Twenty-Fifth Anniversary"] = "25th"
    nameLookupDict["Fifth Edition (Companion)"] = "V5C"
    nameLookupDict["Print on Demand"] = "POD"
    nameLookupDict["Promo"] = "Promo"
    try:
        return nameLookupDict[name]
    except KeyError:
        print(f"Name unknown: {name}")
        return "XXX"

def find_sets(jobj):
    found_sets=[]
    unique_set_names=[]
    for set_ in [x["sets"] for x in jobj]:
        for set_name in set_.keys():
            if "promo" in set_name.lower() or "2015 Storyline Rewards" == set_name or set_name == "2018 Humble Bundle":
                set_name = 'Promo'

            try:
                release_date = set_[set_name][0]["release_date"]
            except KeyError:
                release_date = None
            if set_name in unique_set_names:
                continue
            unique_set_names.append(set_name)
            short_name = get_short_name(set_name)
            found_sets.append({'release_date': release_date, 'short_name': short_name, 'name': set_name})

    return found_sets

class Card():
    def __init__(self, jobj):
        self.name = jobj['name']
        self.printed_name = jobj['printed_name']
        self.types = jobj['types']
        self.is_crypt = "Vampire" in self.types or "Imbeud" in self.types
        self.clans = jobj.get('clans', None)
        self.url = jobj['url']
        self.group = jobj.get('group', None)
        self.capacity = jobj.get('capacity', None)
        self.disciplines = jobj.get('disciplines', None)
        self.banned = jobj.get('banned', None)
        self.pool_cost = jobj.get('pool_cost', None)
        self.blood_cost = jobj.get('blood_cost', None)
        self.sets = [get_short_name(set_) for set_ in jobj['sets']]
        self.scans = {}
        for scan in jobj['scans']:
            self.scans[get_short_name(scan)] = jobj['scans'][scan]
        self.rulings = jobj.get('rulings', None)

    def __str__(self):
        str_  = ""
        str_ += f"Name: {self.name}, Printed: {self.printed_name}, Types: {self.types}, Clans: {self.clans},"
        str_ += f"Group: {self.group}, Capacity: {self.capacity}, Disciplines: {self.disciplines},"
        str_ += f"Sets: {self.sets}, Scans: {self.scans}, Banned: {self.banned}, Blood Cost: {self.blood_cost},"
        str_ += f"Pool Cost: {self.pool_cost}, Rulings: {self.rulings}"
        return str_


def find_cards(jobj):
    found_cards = []
    for jcard in jobj:
        c = Card(jcard)
        print(f"Card: {c}")
        found_cards.append(c.__dict__)
    return found_cards

def create_wmrh():
    wmrh = {'meta' :[], 'sets': [], 'cards': []}
    wmrh['meta'] = {"generate_date": datetime.now().strftime("%m/%d/%Y, %H:%M:%S")}
    wmrh['version'] = '1.0'
    return wmrh

if __name__ == '__main__':
    jobj = fetch_jobj()
    wmrh = create_wmrh()
    wmrh['sets'] = find_sets(jobj)
    wmrh['cards'] = find_cards(jobj)

    with open("wmrh.json", "w") as fd:
        fd.write(json.dumps(wmrh))
