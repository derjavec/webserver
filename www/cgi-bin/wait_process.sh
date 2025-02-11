#!/bin/bash
echo "Content-Type: text/plain"
# Déclaration des hexagrammes et leurs noms
hexagrammes=(
    "䷀ 乾為天	Le créateur"
    "䷁ 坤為地	Le réceptif"
    "䷂ 水雷屯	La difficulté initiale"
    "䷃ 山水蒙	La folie juvénile"
    "䷄ 水天需	L'attente"
    "䷅ 天水訟	Le conflit"
    "䷆ 地水師	L'armée"
    "䷇ 水地比	La solidarité, l'union"
    "䷈ 風天小畜	Le pouvoir d'apprivoisement du petit"
    "䷉ 天澤履	La marche"
    "䷊ 地天泰	La paix"
    "䷋ 天地否	La stagnation, l'immobilité"
    "䷌ 天火同人	Communauté avec les hommes"
    "䷍ 火天大有	Le grand avoir"
    "䷎ 地山謙	L'humilité"
    "䷏ 雷地豫	L'enthousiasme"
    "䷐ 澤雷隨	La suite"
    "䷑ 山風蠱	Le travail sur ce qui est corrompu"
    "䷒ 地澤臨	L'approche"
    "䷓ 風地觀	La contemplation"
    "䷔ 火雷噬嗑	Mordre au travers"
    "䷕ 山火賁	La grâce"
    "䷖ 山地剝	L'éclatement"
    "䷗ 地雷復	Le retour"
    "䷘ 天雷无妄	L'innocence"
    "䷙ 山天大畜	Le pouvoir d'apprivoisement du grand"
    "䷚ 山澤損	La diminution"
    "䷛ 雷益	La prépondérance du grand"
    "䷜ 澤天夬	La percée (la résolution)"
    "䷝ 天風姤	Venir à la rencontre"
    "䷞ 澤地萃	Le rassemblement (le recueillement)"
    "䷟ 地風升	La poussée vers le haut"
    "䷠ 雷風恒	La constance"
    "䷡ 天山遯	La retraite"
    "䷢ 雷天大壯	La puissance du grand"
    "䷣ 火地晉	Le progrès"
    "䷤ 地火明夷	L'obscurcissement de la lumière"
    "䷥ 風火家人	La famille (le clan)"
    "䷦ 火山旅	Le voyage"
    "䷧ 巽為風	Le doux (le pénétrant, le vent)"
    "䷨ 為澤	Le serein, le joyeux, le lac"
    "䷩ 風水渙	La dissolution (la dispersion)"
    "䷪ 山蹇	L'obstacle"
    "䷫ 水解	La libération"
    "䷬ 澤火革	La révolution, la mue"
    "䷭ 火風鼎	Le chaudron"
    "䷮ 雷山小過	La petite traversée"
    "䷯ 山雷頤	La nourriture"
    "䷰ 澤風大	過La grande traversée"
    "䷱ 坎為水	L'insondable, l'eau"
    "䷲ 離為火	Ce qui s'attache, le feu"
    "䷳ 澤山咸	L'influence (la demande en mariage)"
    "䷴ 山澤蹇	La durée"
    "䷵ 風澤中	孚La vérité intérieure"
    "䷶ 雷風益	La prépondérance du petit"
    "䷷ 地天升	Le développement (le progrès graduel)"
    "䷸ 火明夷	L'épousée"
    "䷹ 火山旅	L'abondance, la plénitude"
    "䷺ 地山謙	Le voyageur"
    "䷻ 火地晉	Avant l'accomplissement"
    "䷼ 天火同	人Après l'accomplissement"
)

# Fonction pour sélectionner un hexagramme aléatoire
generer_hexagramme_aleatoire() {
    local total=${#hexagrammes[@]}
    local index=$((RANDOM % total))
    echo "${hexagrammes[$index]}"
}

# Programme principal
main() {
	echo "乾   ☰   Ciel  坤    ☷   Terre  震           ☳  Tonnerre  巽   ☴ Vent/Bois"
	echo "坎  ☵   Eau  離     ☲   Feu  艮           ☶ Montagne 兌    だ ☱ Lac"
	generer_hexagramme_aleatoire
}

# Lancer le programme
main

