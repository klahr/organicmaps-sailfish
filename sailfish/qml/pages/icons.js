.pragma library

// The icon of a search category, in its night version for a dark ambience.
function category(key, dark) {
    return Qt.resolvedUrl("../../icons/categories/ic_" + key + (dark ? "_night" : "") + ".svg")
}
