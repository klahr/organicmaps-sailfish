.pragma library

// Returns to the map page, e.g. after choosing something to show on the map.
function popToMap(pageStack) {
    pageStack.pop(pageStack.find(function(page) { return page.objectName === "mapPage" }))
}
