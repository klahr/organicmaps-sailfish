import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0

// Roads to avoid on car routes and stop reordering, like the Android Routing options screen.
Page {
    property QtObject routing

    allowedOrientations: Orientation.All

    function setAvoided(road, avoid) {
        routing.avoidRoads = avoid ? (routing.avoidRoads | road) : (routing.avoidRoads & ~road)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("driving_options_title")
            }
            Repeater {
                model: [
                    { road: Routing.Toll, text: appInfo.localized("avoid_tolls") },
                    { road: Routing.Dirty, text: appInfo.localized("avoid_unpaved") },
                    { road: Routing.Ferry, text: appInfo.localized("avoid_ferry") },
                    { road: Routing.Motorway, text: appInfo.localized("avoid_motorways") }
                ]

                TextSwitch {
                    text: modelData.text
                    checked: (routing.avoidRoads & modelData.road) !== 0
                    onClicked: setAvoided(modelData.road, checked)
                }
            }
            SectionHeader {
                text: appInfo.localized("route_optimization")
            }
            TextSwitch {
                text: appInfo.localized("route_optimization")
                description: appInfo.localized("route_optimization_description")
                checked: routing.routeOptimization
                onClicked: routing.routeOptimization = checked
            }
        }
    }
}
