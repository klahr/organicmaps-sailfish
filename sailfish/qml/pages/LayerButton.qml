import QtQuick 2.6
import Sailfish.Silica 1.0

// Map layer preview with its name; the outline marks an enabled layer, like on Android.
BackgroundItem {
    id: layerButton

    property alias source: preview.source
    property alias text: label.text
    property bool checked

    height: column.height + 2 * Theme.paddingSmall

    Column {
        id: column
        y: Theme.paddingSmall
        width: parent.width
        spacing: Theme.paddingSmall

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            // Close to the on-screen size of the Android previews, plus room for the outline.
            width: Theme.itemSizeMedium + 2 * Theme.paddingSmall
            height: width

            Image {
                id: preview
                // The corner radius of the Android previews, which Qt's SVG Tiny renderer drops with their clip path.
                readonly property real cornerRadius: width * 10 / 54

                anchors.fill: parent
                anchors.margins: Theme.paddingSmall
                sourceSize: Qt.size(width, height)
                layer.enabled: true
                layer.effect: ShaderEffect {
                    property real radius: preview.cornerRadius / preview.width
                    fragmentShader: "
                        varying highp vec2 qt_TexCoord0;
                        uniform sampler2D source;
                        uniform lowp float qt_Opacity;
                        uniform highp float radius;
                        void main() {
                            highp vec2 corner = max(abs(qt_TexCoord0 - 0.5) - (0.5 - radius), 0.0);
                            lowp float inside = 1.0 - smoothstep(radius - 0.02, radius, length(corner));
                            gl_FragColor = texture2D(source, qt_TexCoord0) * inside * qt_Opacity;
                        }"
                }
            }

            // Around the preview, its corners following the preview's.
            Rectangle {
                anchors.fill: parent
                radius: preview.cornerRadius + Theme.paddingSmall
                color: "transparent"
                border.color: Theme.highlightColor
                border.width: Theme.dp(2)
                visible: layerButton.checked
            }
        }

        Label {
            id: label
            x: Theme.paddingSmall
            width: parent.width - 2 * x
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            maximumLineCount: 2
            font.pixelSize: Theme.fontSizeExtraSmall
            color: layerButton.checked || layerButton.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
